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

#include <stdint.h>
#include <stdio.h>
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
	uint8_t r, g, b, v = 0;

	for (;;) {
		i = (i + 1) % 360;
		hsv(&r, &g, &b, i, 255, v);

		cli();
		a->r = r;
		a->g = g;
		a->b = b;
		v = *a->v;		/* 10bit to 8bit */
		sei();

		sleep(MSEC(40));
	}
}

void
pwm(void *arg)
{
	struct pwmarg *a = (struct pwmarg *)arg;
	uint32_t t;
	uint8_t v;

	DDRB |= _BV(a->pin);
	PORTB &= ~_BV(a->pin);

#define DIV	(UINT8_MAX >> 1)

	for (;;) {
		cli();
		v = *a->value;
		sei();

		/* on */
		if ((t = SEC2(v) / DIV)) {
			PORTB |= _BV(a->pin);
			sleep(t);
		}

		/* off */
		if ((t = SEC2(UINT8_MAX - v) / DIV)) {
			PORTB &= ~_BV(a->pin);
			sleep(t);
		}
	}
}
