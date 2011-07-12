/* $Id$ */
/*
 * Copyright (c) 2011 Dimitri Sokolyuk <demon@dim13.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define USE_DELAY

#include <inttypes.h>
#include <avr/io.h>
#ifdef USE_DELAY
#warning "using dalay routines"
#include <util/delay.h>
#endif
#include "kernel.h"
#include "tasks.h"

#define CLEAR_DISPLAY		_BV(0)
#define RETURN_HOME		_BV(1)

#define ENTRY_MODE_SET		_BV(2)
#define INC_DDRAM		_BV(1)
#define SHIFT_ENTIRE_DISP	_BV(0)

#define ON_OFF_CONTROL		_BV(3)
#define DISPLAY_ON		_BV(2)
#define CURSOR			_BV(1)
#define BLINK			_BV(0)

#define CURSOR_OR_DISPLAY_SHIFT	_BV(4)
#define SHIFT_CURS_LEFT		(0x00 << 2)
#define SHIFT_CURS_RIGHT	(0x01 << 2)
#define SHIFT_DISP_LEFT		(0x10 << 2)
#define SHIFT_DISP_RIGHT	(0x11 << 2)

#define FUNCTION_SET		_BV(5)
#define DATA_LENGTH_8BIT	_BV(4)	/* on 8 / off 4 bit */
#define TWO_LINES		_BV(3)	/* on 2 / off 1 line */
#define BIG_FONT		_BV(2)	/* on 5x10 / off 5x7 */


#define SET_CGRAM_ADDRESS	_BV(6)
#define SET_DDRAM_ADDRESS	_BV(7)
#define BUSY_FLAG		_BV(7)

/* 3-wire interface with 74HC164 */
#define PORT			PORTD
#define PORTDIR			DDRD
#define	DATA			PD5
#define	CLOCK			PD6
#define	E			PD7

#ifdef USE_DELAY
#define write_cmd(x, delay)	do { write_byte((x), 0); _delay_us(delay); } while (0)
#define write_data(x)		do { write_byte((x), 1); _delay_us(43); } while (0)
#else
#define write_cmd(x, delay)	do { write_byte((x), 0); sleep(SOFT, USEC(delay)); } while (0)
#define write_data(x)		do { write_byte((x), 1); sleep(SOFT, USEC(43)); } while (0)
#endif
#define move(line, row)		do { write_cmd(SET_DDRAM_ADDRESS | ((line) << 6) | (row), 39); } while (0)
#define clear()			do { write_cmd(CLEAR_DISPLAY, 1530); } while (0)
#define home()			do { write_cmd(RETURN_HOME, 1530); } while (0)


/* recomended cycle 1us: 450ns on, 450ns off. this is beyond our resolution */
#define wait_short()		do { asm volatile ("nop"); } while (0)
#define strobe(port, bit)	do { port |= _BV(bit); wait_short(); port &= ~_BV(bit); } while (0)
#define setif(cond, port, bit)	do { if (cond) port |= _BV(bit); else port &= ~_BV(bit); } while (0)

static void
write_byte(uint8_t byte, uint8_t rs)
{
	uint8_t i;

	for (i = 0; i < 8; i++) {
		setif(byte & (0b10000000 >> i), PORT, DATA);	/* MSF */
		strobe(PORT, CLOCK);
	}
	setif(rs, PORT, DATA);
	strobe(PORT, E);
}

static void
mvputs(uint8_t line, uint8_t row, char *s)
{
	move(line, row);
	while (*s)
		write_data(*(s++));
}

static void
mvputch(uint8_t line, uint8_t row, char ch)
{
	move(line, row);
	write_data(ch);
}

#if 0
static char *
itoa(int32_t n)
{
	static char buf[12];
	char *s = &buf[11];
	uint32_t sign, u = n;

	*s = '\0';

	if ((sign = (n < 0)))
		u = ~u + 1;

	do
		*--s = '0' + (u % 10);
	while ((u /= 10) > 0);

	if (sign)
		*--s = '-';

	while (s != buf)
		*--s = ' ';

	return s;
}

static char hex[] = "0123456789ABCDEF";

static char *
itohex16(uint16_t x)
{
	static char buf[5];
	char *s = &buf[4];
	uint8_t i;

	*s = '\0';

	for (i = 0; i < 16; i += 4)
		*--s = hex[(x >> i) & 0x0f];

	return s;
}


static char *
itohex32(uint32_t x)
{
	static char buf[9];
	char *s = &buf[8];
	uint8_t i;

	*s = '\0';

	for (i = 0; i < 32; i += 4)
		*--s = hex[(x >> i) & 0x0f];

	return s;
}
#endif

void
lcd(void *arg)
{
	struct lcdarg *a = (struct lcdarg *)arg;

	PORTDIR |= (_BV(DATA) | _BV(CLOCK) | _BV(E));

	/* task init: wait >40ms */
	update(MSEC(40), MSEC(500));

	/* 8 bit, 2 line, 5x8 font */
	write_cmd(FUNCTION_SET | DATA_LENGTH_8BIT | TWO_LINES, 39);
	/* no BUSY_FLAG until now */

	/* display on, cursor off, cursor blink off */
	write_cmd(ON_OFF_CONTROL | DISPLAY_ON, 39);

	/* clear display */
	clear();

	/* entry mode */
	write_cmd(ENTRY_MODE_SET | INC_DDRAM, 39);

	home();

	*a->first = '\0';
	*a->second = '\0';

	for (;;) {
		mvputs(0, 0, a->first);
		mvputs(1, 0, a->second);
		sleep(SOFT, MSEC(40));	/* 25 Hz */
	}
}
