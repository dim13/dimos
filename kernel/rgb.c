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
		//hsv(&r, &g, &b, i, v, v);
		hsv(&r, &g, &b, i, 255, 255);

		wait(1);
		a->r = r;
		signal(1);

		wait(2);
		a->g = g;
		signal(2);

		wait(3);
		a->b = b;
		signal(3);

		v = *a->v >> 2;		/* 10bit to 8bit */
		//sei();

		sleep(MSEC(40));
	}
}

void
pwm(void *arg)
{
	struct pwmarg *a = (struct pwmarg *)arg;
	uint32_t on, off;
	uint8_t v;

	DDRB |= _BV(a->pin);
	PORTB &= ~_BV(a->pin);

#define DIV	(UINT8_MAX >> 1)

	for (;;) {
		//cli();
		wait(a->sema);
		v = *a->value;
		signal(a->sema);
		//sei();

		if ((on = SEC2(v) / DIV)) {
			PORTB |= _BV(a->pin);
			sleep(on);
		}

		if ((off = SEC2(UINT8_MAX - v) / DIV)) {
			PORTB &= ~_BV(a->pin);
			sleep(off);
		}
	}
}
