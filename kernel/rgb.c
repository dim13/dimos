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

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "kernel.h"
#include "tasks.h"

void hsv(uint8_t *, uint8_t *, uint8_t *, uint16_t, uint8_t, uint8_t);

void
rgb(void *arg)
{
	struct rgbarg *a = (struct rgbarg *)arg;
	uint16_t i = 0;
	uint8_t r, g, b, v;

#define SCALE	1

	cli();
	a->m = 255 >> SCALE;
	sei();

	update(0, MSEC(50));

	for (;;) {
		i = (i + 1) % 360;
		hsv(&r, &g, &b, i, v, v);

		cli();
		a->r = r >> SCALE;
		a->g = g >> SCALE;
		a->b = b >> SCALE;
		v = *a->v >> 2;		/* 10bit to 8bit */
		sei();

		update(MSEC(40), MSEC(50));
	}
}

void
pwm(void *arg)
{
	struct pwmarg *a = (struct pwmarg *)arg;
	uint16_t maxval, on, off;

	DDRB |= _BV(a->pin);
	PORTB &= ~_BV(a->pin);

#define DL	SEC4(5)

	cli();
	maxval = *a->mval;
	sei();
	update(0, DL);

	for (;;) {
		cli();
		on = *a->value;
		sei();

		if (on) {
			PORTB |= _BV(a->pin);
			update(SEC4(on), DL);
		}

		off = maxval - on;
		if (off) {
			PORTB &= ~_BV(a->pin);
			update(SEC4(off), DL);
		}
	}
}
