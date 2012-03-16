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

#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
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
#define SHIFT_CURS_LEFT		0
#define SHIFT_CURS_RIGHT	_BV(2)
#define SHIFT_DISP_LEFT		_BV(3)
#define SHIFT_DISP_RIGHT	(_BV(2) | _BV(3))

#define FUNCTION_SET		_BV(5)
#define DATA_LENGTH_8BIT	_BV(4)	/* on 8 / off 4 bit */
#define TWO_LINES		_BV(3)	/* on 2 / off 1 line */
#define BIG_FONT		_BV(2)	/* on 5x10 / off 5x7 */

#define SET_CGRAM_ADDRESS	_BV(6)
#define SET_DDRAM_ADDRESS	_BV(7)
#define BUSY_FLAG		_BV(7)

/* 3-wire interface to HD44780 throuth 74HC164 */
#define PORT			PORTD
#define PORTDIR			DDRD
#define DATA			PD5
#define CLOCK			PD6
#define E			PD7

/* recomended cycle: 450ns on, 450ns off. this is beyond our resolution */
#define strobe(port, bit)	do { port |= _BV(bit); _NOP(); port &= ~_BV(bit); } while (0)
#define setif(cond, port, bit)	do { if (cond) (port) |= _BV(bit); else (port) &= ~_BV(bit); } while (0)
#define write_data(x)		do { write_byte((x), 1); _delay_us(43); } while (0)
#define _write_cmd(x, delay)	do { write_byte((x), 0); _delay_us(delay); } while (0)
#define write_cmd(x)		_write_cmd((x), 39)
#define move(line, row)		_write_cmd(SET_DDRAM_ADDRESS | ((line) << 6) | (row), 39)
#define clear()			_write_cmd(CLEAR_DISPLAY, 1530)
#define home()			_write_cmd(RETURN_HOME, 1530)

static void
write_byte(uint8_t byte, uint8_t rs)
{
	uint8_t i;

	for (i = 0; i < 8; i++) {
		setif(byte & (0x80 >> i), PORT, DATA); /* MSF */
		strobe(PORT, CLOCK);
	}

	setif(rs, PORT, DATA);
	strobe(PORT, E);
}

void
lcd_init(void)
{
	FILE *lcd_stream;

	PORTDIR |= (_BV(DATA) | _BV(CLOCK) | _BV(E));

	/* task init: wait >40ms */
	_delay_ms(40);

	/* 8 bit, 2 line, 5x8 font */
	write_cmd(FUNCTION_SET | DATA_LENGTH_8BIT | TWO_LINES);
	/* no BUSY_FLAG until now */

	/* display on, cursor off, cursor blink off */
	write_cmd(ON_OFF_CONTROL | DISPLAY_ON);

	/* clear display */
	clear();

	/* entry mode */
	write_cmd(ENTRY_MODE_SET | INC_DDRAM);

	home();

	lcd_stream = fdevopen(lcd_putchar, NULL);
	stderr = lcd_stream;
}

int
lcd_putchar(char c, FILE *fd)
{
	switch (c) {
	case '\b':	/* XXX */
		clear();
		/* FALLTHROUGH */
	case '\r':
		home();
		break;
	case '\n':
		move(1, 0);
		break;
	case '\t':
		c = ' ';
		/* FALLTHROUGH */
	default:
		write_data(c);
		break;
	}

	return 0;
}
