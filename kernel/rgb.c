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

#include <avr/io.h>

#include <stdint.h>
#include <stdio.h>

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
		lock(Adc);
		v = *a->v;		/* 8bit */
		unlock(Adc);

		i = (i + 1) % 360;
		hsv(&r, &g, &b, i, 255, v);

		lock(RGB);
		a->r = r;
		a->g = g;
		a->b = b;
		unlock(RGB);

		sleep(0, 234375);
	}
}

#if 0
#define	SCALE(x)	(((x) << 6) + ((x) << 4))	/* 50Hz */
#else
#define	SCALE(x)	((x) << 6)			/* 60Hz */
#endif

void
pwm(void *arg)
{
	struct pwmarg *a = (struct pwmarg *)arg;
	uint32_t t;

	DDRB |= _BV(a->pin);
	PORTB &= ~_BV(a->pin);

	for (;;) {
		lock(RGB);
		t = *a->value;
		unlock(RGB);

		/* on */
		if (t) {
			PORTB |= _BV(a->pin);
			sleep(0, SCALE(t));
		}

		/* off */
		if ((t = UINT8_MAX - t)) {
			PORTB &= ~_BV(a->pin);
			sleep(0, SCALE(t));
		}
	}
}
