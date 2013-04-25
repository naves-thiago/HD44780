#include "hd44780.h"
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


#define BUSY_FLAG	(1<<7)

typedef struct sPin
{
  pio_type pin, port;
} tPin;

unsigned char display_config;
tPin P_RS, P_RW, P_EN, P_D4, P_D5, P_D6, P_D7;

/* Internal usage functions. */
static void write_nibble (unsigned char c);
static unsigned char read_nibble (void);
static void delay_ms (unsigned char t);
static tPin convertPin( int p );
static void setPinVal( tPin p, char val );
static void setPinDir( tPin p, char dir );
static int getPinVal( tPin p );

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

void hd44780_init (void)
{
	hd44780h_dir_nibble( DIR_OUT );
	hd44780h_rw(0);
	hd44780h_rs(0);

	/* software reset */
	delay_ms (15);
	write_nibble (0x03);
	delay_ms (4);
	write_nibble (0x03);
	delay_ms (1);
	write_nibble (0x03);
	delay_ms (1);
	/* we're in 8-bits mode, set it to 4-bits */
	write_nibble (0x02);
	/* busy flag can be checked from now on... */

	hd44780_write_cmd (0x28); /* bits:4, lines: 2, font: 5x8 */
	hd44780_control (1, 0, 0);
//	hd44780_write_cmd (0x0E); /* disp:on, cursor:on, blink:off */
	hd44780_write_cmd (0x01); /* clear */
}

/* rs is normaly 0, and rw is normaly 0, so no need to change here. */
void hd44780_write_data (unsigned char c)
{
	while (hd44780_read_status() & BUSY_FLAG)
		;
	write_nibble(c >> 4);
	write_nibble(c);
}

void hd44780_write_cmd (unsigned char c)
{
	while (hd44780_read_status() & BUSY_FLAG)
		;
	hd44780h_rs(0);
	write_nibble(c >> 4);
	write_nibble(c);
	hd44780h_rs(1);
}

unsigned char hd44780_read_data (unsigned char c)
{
	unsigned char readed;
	while (hd44780_read_status() & BUSY_FLAG)
		;
	hd44780h_rw(1);
	hd44780h_dir_nibble( DIR_IN );
	readed = (read_nibble() << 4) | (read_nibble() & 0x0F);
	hd44780h_rw(0);
	hd44780h_dir_nibble( DIR_OUT );
	return readed;
}

unsigned char hd44780_read_status ()
{
	unsigned char readed;
	hd44780h_rw(1);
	hd44780h_rs(0);
	hd44780h_dir_nibble( DIR_IN );
	readed = ((read_nibble() << 4) & 0xF0) | (read_nibble() & 0x0F);
	hd44780h_rw(0);
	hd44780h_rs(1);
	hd44780h_dir_nibble( DIR_OUT );
	return readed;
}

static void write_nibble (unsigned char c)
{
	hd44780h_en(1);
	hd44780h_set_nibble(c);
	hd44780h_en(0);
}

static unsigned char read_nibble (void)
{
	unsigned char c;
	hd44780h_en(1);
	c = hd44780h_get_nibble();
	hd44780h_en(0);
	return c;
}

static void delay_ms (unsigned char t)
{
  platform_timer_delay( 1, 1000*t );
}

void hd44780h_en (unsigned char set)
{
  setPinVal( P_EN, set );
}

void hd44780h_rs (unsigned char set)
{
  setPinVal( P_RS, set );
}

void hd44780h_rw (unsigned char set)
{
  setPinVal( P_RW, set );
}

void hd44780h_set_nibble (unsigned char data)
{
  setPinVal( P_D4, data & 0x01 );
  setPinVal( P_D5, data & 0x02 );
  setPinVal( P_D6, data & 0x04 );
  setPinVal( P_D7, data & 0x08 );
}

unsigned char hd44780h_get_nibble (void)
{
  unsigned char tmp = 0; 
  tmp = getPinVal( P_D4 ) & 0x01;
  tmp |= ( getPinVal( P_D5 ) & 0x01 ) << 1;
  tmp |= ( getPinVal( P_D6 ) & 0x01 ) << 2;
  tmp |= ( getPinVal( P_D7 ) & 0x01 ) << 3;
  return tmp;
}

void hd44780h_dir_nibble (unsigned char in)
{
  setPinDir( P_D4, in );
  setPinDir( P_D5, in );
  setPinDir( P_D6, in );
  setPinDir( P_D7, in );
}

void hd44780_write( char * str )
{
   int i;
   for ( i=0; str[i] != '\0'; i++ )
     hd44780_write_data( str[i] );
}

/* Lua: hd44780.write( "string" ) */
static int hd44780_write_lua( lua_State *L )
{
   char * str = luaL_checkstring( L, 1 );
   hd44780_write( str ); 
   return 0;
}

void hd44780_goto( char x, char y )
{
  // go to 0,0
  hd44780_write_cmd( 0x02 );

  int qtd = x + HD44780_WIDTH * y;

  // Go X chars to the right
  int i;
  for ( i=0; i < qtd; i++ )
    hd44780_write_cmd( 0x14 );
}

/* Lua: hd44780_goto( x, y ) */
static int hd44780_goto_lua( lua_State *L )
{
  hd44780_goto( luaL_checkinteger( L, 1 ), luaL_checkinteger( L, 2 ) );
  return 0;
}

/* Pins: P_RS, P_RW, P_EN, P_D4, P_D5, P_D6, P_D7; */
static int hd44780_init_lua( lua_State *L )
{
  P_RS = convertPin( luaL_checkinteger( L, 1 ) );
  P_RW = convertPin( luaL_checkinteger( L, 2 ) );
  P_EN = convertPin( luaL_checkinteger( L, 3 ) );
  P_D4 = convertPin( luaL_checkinteger( L, 4 ) );
  P_D5 = convertPin( luaL_checkinteger( L, 5 ) );
  P_D6 = convertPin( luaL_checkinteger( L, 6 ) );
  P_D7 = convertPin( luaL_checkinteger( L, 7 ) );

  setPinDir( P_RS, DIR_OUT ); 
  setPinDir( P_RW, DIR_OUT ); 
  setPinDir( P_EN, DIR_OUT ); 
  setPinDir( P_D4, DIR_OUT ); 
  setPinDir( P_D5, DIR_OUT ); 
  setPinDir( P_D6, DIR_OUT ); 
  setPinDir( P_D7, DIR_OUT ); 

  hd44780_init();
  return 0;
}



const LUA_REG_TYPE hd44780_map[] = {
  { LSTRKEY( "init" ), LFUNCVAL( hd44780_init_lua ) },
  { LSTRKEY( "write" ), LFUNCVAL( hd44780_write_lua ) },
  { LSTRKEY( "goto" ), LFUNCVAL( hd44780_goto_lua ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_hd44780( lua_State *L )
{
  LREGISTER( L, "hd44780", hd44780_map );
  return 1;
};

