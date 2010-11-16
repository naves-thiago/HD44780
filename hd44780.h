/** @file
 * @defgroup hd44780 HD44780 display driver
 * @{
*/

#ifndef __HD44780_H_
#define __HD44780_H_

/**************************************************************************
			Interface
**************************************************************************/
void		hd44780_init (void);
void		hd44780_write_data (unsigned char c);
void		hd44780_write_cmd (unsigned char c);
unsigned char	hd44780_read_data (unsigned char c);
unsigned char	hd44780_read_status (void);
void hd44780_write( char * str );
void hd44780_goto( char x, char y );

/**************************************************************************
		User Defined Functions
**************************************************************************/

/** @defgroup hd44780_user_defined User Defined Functions
 * @{ */

/* get the mcu pins as output */
void hd44780h_init_hw (void);

/* 1 for input, 0 for output */
void hd44780h_dir_nibble (unsigned char in);

/* bit 0 from display must be bit 0 from variable returned value */
void hd44780h_set_nibble (unsigned char data);
unsigned char hd44780h_get_nibble (void);

/* 1 for set, 0 for clear */
void hd44780h_en (unsigned char set);
void hd44780h_rs (unsigned char set);
void hd44780h_rw (unsigned char set);

/** @} */
/**************************************************************************
		End of User Defined Functions
**************************************************************************/

#define HD44780_WIDTH		16
#define HD44780_HEIGHT		2
#define HD44780_LINE0		0x00
#define HD44780_LINE1		0x40
#define HD44780_LINE2		0x14
#define HD44780_LINE3		0x54

/**************************************************************************
			Display Commands
**************************************************************************/

#define hd44780_clear()		hd44780_write_cmd (0x01)
#define hd44780_home()		hd44780_write_cmd (0x02)

/** @param id - cursor increment/decrement direction.
 * @param s - cursor seems still & display shifts. */
#define hd44780_mode(id, s)	hd44780_write_cmd\
	(0x04			\
	| ((id) ? 0x02 : 0)	\
	| ((s) ? 0x01 : 0))

/** @param d - display on/!off
 * @param c - cursor on/!off
 * @param b - cursonr blinking on/!off */
#define hd44780_control(d, c, b) hd44780_write_cmd\
	(0x08			\
	| ((d) ? 0x04 : 0)	\
	| ((c) ? 0x02 : 0)	\
	| ((b) ? 0x01 : 0))

/** @param sc - shift display without changing its content.
 * @param rl - */
#define hd44780_shift(s, r)	hd44780_write_cmd\
	(0x10			\
	| ((sc) ? 0x08 : 0)	\
	| ((rl) ? 0x04 : 0))

#define hd44780_fnset(d, n, f)	hd44780_write_cmd\
	(0x20			\
	| ((d) ? 0x10 : 0)	\
	| ((n) ? 0x08 : 0)	\
	| ((f) ? 0x04 : 0))

#define hd44780_cgram_adr(adr)	hd44780_write_cmd\
	(0x40			\
	| ((adr) & 0x3F))

#define hd44780_ddram_adr(adr)	hd44780_write_cmd\
	(0x80			\
	| ((adr) & 0x7F))

#endif /* __HD44780_H_ */
/** @} */ /*  @defgroup hd44780 */
