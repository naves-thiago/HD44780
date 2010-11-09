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

typedef struct sPin
{
  pio_type pin, port;
} tPin;

tpin P_RS, P_RW, P_EN, P_D0, P_D1, P_D2, P_D3, P_D4, P_D5, P_D6, P_D7;
unsigned char display_config = 0x00;

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
  unsigned char out = 0x0f & data
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

void display_control_set_on( unsigned char flags )
{
    display_config |= flags;
    send_byte( DISPLAY_CONTROL | display_config, 1 );
}

void display_control_set_off( unsigned char flags )
{
    display_config &= ~flags;
    send_byte( DISPLAY_CONTROL | display_config, 1 );
}

void lcd_write( char * str )
{
    int i;

    for (i = 0; str[i]!= '\0'; i++)
        send_byte(str[i], 0);
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

  return 0;
}

const LUA_REG_TYPE keyboard_map[] = {
  { LSTRKEY( "init" ), LFUNCVAL( hd44780_init ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_keyboard ( lua_State *L )
{
  LREGISTER( L, "keyboard", keyboard_map );
};

