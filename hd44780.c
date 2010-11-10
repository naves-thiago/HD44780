#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"
#include "platform_conf.h"
#include <string.h>

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"

#define DIR_OUT 0
#define DIR_IN 1

#define IGNORE 1
#define USE 0

// Commands
#define CLEAR_DISPLAY   0x01
#define RETURN_HOME     0x02
#define DISPLAY_CONTROL 0x08
#define FUNCTION_SET    0x20
#define SET_CGRAM_ADDR  0x40
#define SET_DDRAM_ADDR  0x80

// Flags
#define DISPLAY_ON  0x04
#define CURSOR_ON   0x02
#define BLINKING_ON 0x01
// 4-bit mode
#define DL (0 << 4)
// 2-line display
#define N  (1 << 3)
// 5x8 character font
#define F  (0 << 2)

// Functions
#define lcd_clear()         send_byte( CLEAR_DISPLAY, 1 )
#define lcd_return_home()   send_byte( RETURN_HOME, 1 )
 
#define lcd_display_on()    display_control_set_on( DISPLAY_ON )
#define lcd_display_off()   display_control_set_off( DISPLAY_ON )
#define lcd_cursor_on()     display_control_set_on( CURSOR_ON )
#define lcd_cursor_off()    display_control_set_off( CURSOR_ON )
#define lcd_blinking_on()   display_control_set_on( BLINKING_ON )
#define lcd_blinking_off()  display_control_set_off( BLINKING_ON )

typedef struct sPin
{
  pio_type pin, port;
} tPin;

unsigned char display_config;
tPin P_RS, P_RW, P_EN, P_D0, P_D1, P_D2, P_D3, P_D4, P_D5, P_D6, P_D7;

/* Converts a pin number got from the Lua stack to the tPin format */
static tPin convertPin( int p )
{
  tPin result;
  result.port = PLATFORM_IO_GET_PORT( p );
  result.pin = ( 1 << PLATFORM_IO_GET_PIN( p ) );

  return result;
}

/* Just to make the code easier to read
 * Sets a pin value ( 1 / 0 ) 
 */
static void setPinVal( tPin p, char val )
{
  if ( val )
    platform_pio_op( p.port, p.pin, PLATFORM_IO_PIN_SET );
  else
    platform_pio_op( p.port, p.pin, PLATFORM_IO_PIN_CLEAR );
}

/* Just to make the code easier to read
 * Sets a pin direction ( DIR_IN / DIR_OU )
 */
static void setPinDir( tPin p, char dir )
{
  if ( dir == DIR_IN )
    platform_pio_op( p.port, p.pin, PLATFORM_IO_PIN_DIR_INPUT );
  else
    platform_pio_op( p.port, p.pin, PLATFORM_IO_PIN_DIR_OUTPUT );
}

/* Just to make the code easier to read
 * Returns the current pin value 
 */
static int getPinVal( tPin p )
{
  return platform_pio_op( p.port, p.pin, PLATFORM_IO_PIN_GET );
}

static void send_nibble( unsigned char data )
{
  unsigned char out = 0x0f & data;
  setPinVal( P_D0, out & 0x01 );
  setPinVal( P_D1, out & 0x02 );
  setPinVal( P_D2, out & 0x04 );
  setPinVal( P_D3, out & 0x08 );
  setPinVal( P_D4, out & 0x10 );
  setPinVal( P_D5, out & 0x20 );
  setPinVal( P_D6, out & 0x40 );
  setPinVal( P_D7, out & 0x80 );

  platform_timer_delay( 1, 1000 ); /* 1 milisecond */
  setPinVal( P_EN, 1 );
  platform_timer_delay( 1, 1000 ); /* 1 milisecond */
  setPinVal( P_EN, 0 );
  platform_timer_delay( 1, 1000 ); /* 1 milisecond */
}

static void send_byte( unsigned char data, char isCommand )
{
  setPinVal( P_RS, !(isCommand & 0x01) );
  send_nibble( data >> 4 );
  send_nibble( data );
}

static void display_control_set_on( unsigned char flags )
{
  display_config |= flags;
  send_byte( DISPLAY_CONTROL | display_config, 1 );
}

static void display_control_set_off( unsigned char flags )
{
  display_config &= ~flags;
  send_byte( DISPLAY_CONTROL | display_config, 1 );
}

static void lcd_write( char * str )
{
  int i;

  for (i = 0; str[i]!= '\0'; i++)
    send_byte(str[i], 0);
}

static void lcd_goto(unsigned char row, unsigned char col)
{
    unsigned char addr = ((row - 1) * 0x40) + col - 1;
    send_byte( SET_DDRAM_ADDR | addr, 1 );
}

static void lcd_add_character(unsigned char addr, unsigned char * pattern)
{
    int i;

    send_byte( SET_CGRAM_ADDR | addr << 3, 1 );
    for (i = 0; i < 8; i++)
      send_byte( pattern[i], 0 );
}

static void lcd_initialize(void)
{
    setPinVal( P_RS, 0 );
    setPinVal( P_RW, 0 );

    // set 4-bit mode
    send_nibble( 0x02 );

    // function set
    send_byte( FUNCTION_SET | DL | N | F, 1 );

    lcd_display_on();
    lcd_cursor_off();
    lcd_blinking_off();

    lcd_clear();
    lcd_return_home();
}

/* Pins: P_RS, P_RW, P_EN, P_D0, P_D1, P_D2, P_D3, P_D4, P_D5, P_D6, P_D7; */
static int hd44780_init( lua_State *L )
{
  P_RS = convertPin( luaL_checkinteger( L, 1 ) );
  P_RW = convertPin( luaL_checkinteger( L, 2 ) );
  P_EN = convertPin( luaL_checkinteger( L, 3 ) );
  P_D0 = convertPin( luaL_checkinteger( L, 4 ) );
  P_D1 = convertPin( luaL_checkinteger( L, 5 ) );
  P_D2 = convertPin( luaL_checkinteger( L, 6 ) );
  P_D3 = convertPin( luaL_checkinteger( L, 7 ) );
  P_D4 = convertPin( luaL_checkinteger( L, 8 ) );
  P_D5 = convertPin( luaL_checkinteger( L, 9 ) );
  P_D6 = convertPin( luaL_checkinteger( L, 10 ) );
  P_D7 = convertPin( luaL_checkinteger( L, 11 ) );

  setPinDir( P_RS, DIR_OUT ); 
  setPinDir( P_RW, DIR_OUT ); 
  setPinDir( P_EN, DIR_OUT ); 
  setPinDir( P_D0, DIR_OUT ); 
  setPinDir( P_D1, DIR_OUT ); 
  setPinDir( P_D2, DIR_OUT ); 
  setPinDir( P_D3, DIR_OUT ); 
  setPinDir( P_D4, DIR_OUT ); 
  setPinDir( P_D5, DIR_OUT ); 
  setPinDir( P_D6, DIR_OUT ); 
  setPinDir( P_D7, DIR_OUT ); 

  lcd_initialize();

  return 0;
}

/* Lua: hd44780.write( "string" ) */
static int hd44780_write( lua_State *L )
{
   char * str = luaL_checkstring( L, 1 );
   lcd_write( str );
   return 0;
}

/* Lua: hd44780_goto( x, y ) */
static int hd44780_goto( lua_State *L )
{
  lcd_goto( luaL_checkinteger( L, 1 ), luaL_checkinteger( L, 2 ) );
  return 0;
}

const LUA_REG_TYPE hd44780_map[] = {
  { LSTRKEY( "init" ), LFUNCVAL( hd44780_init ) },
  { LSTRKEY( "write" ), LFUNCVAL( hd44780_write ) },
  { LSTRKEY( "goto" ), LFUNCVAL( hd44780_goto ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_hd44780( lua_State *L )
{
  LREGISTER( L, "hd44780", hd44780_map );
  return 1;
};

