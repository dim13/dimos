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
#include <avr/pgmspace.h>
#include "kernel.h"
#include "tasks.h"

void hsv(uint8_t *, uint8_t *, uint8_t *, uint16_t, uint8_t, uint8_t);

// uint8_t factor[] = { 1, 2, 3, 4, 6, 8, 12, 16, 23, 32, 45, 64, 90, 128, 180, 255 };
uint8_t factor[] PROGMEM = {
   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,
   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   2,   2,   2,   2,   2,   2,
   2,   2,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   3,   3,   3,   3,   3,
   4,   4,   4,   4,   4,   4,   4,   4,
   4,   4,   5,   5,   5,   5,   5,   5,
   5,   5,   6,   6,   6,   6,   6,   6,
   6,   6,   7,   7,   7,   7,   7,   7,
   8,   8,   8,   8,   8,   9,   9,   9,
   9,   9,  10,  10,  10,  10,  11,  11,
  11,  11,  12,  12,  12,  12,  13,  13,
  13,  13,  14,  14,  14,  15,  15,  15,
  16,  16,  17,  17,  17,  18,  18,  18,
  19,  19,  20,  20,  21,  21,  22,  22,
  23,  23,  24,  24,  25,  25,  26,  26,
  27,  27,  28,  29,  29,  30,  31,  31,
  32,  33,  34,  34,  35,  36,  37,  37,
  38,  39,  40,  41,  42,  43,  44,  45,
  46,  47,  48,  49,  50,  51,  52,  53,
  54,  55,  57,  58,  59,  61,  62,  63,
  65,  66,  68,  69,  71,  72,  74,  75,
  77,  79,  80,  82,  84,  86,  88,  90,
  92,  94,  96,  98, 100, 102, 104, 107,
 109, 111, 114, 116, 119, 122, 124, 127,
 130, 133, 136, 139, 142, 145, 148, 151,
 154, 158, 161, 165, 168, 172, 176, 180,
 184, 188, 192, 196, 200, 205, 209, 214,
 219, 223, 228, 233, 238, 244, 249, 255,
};

void
rgb(void *arg)
{
	struct rgbarg *a = (struct rgbarg *)arg;
	uint32_t d = deadline();
	uint32_t r = release();
	uint16_t i = 0;
	uint8_t v;

	for (;;) {
		i = (i + 1) % 360;
		v = i % 120;
		v = (v < 60) ? 255 - 2 * v : 15 + 2 * v;

		hsv(a->r, a->g, a->b, i, 255, v);

		r = d;
		d += MSEC(28);
		update(r, d);
	}
}

void
pwm(void *arg)
{
#define	SCALE 6
	struct pwmarg *a = (struct pwmarg *)arg;
	uint32_t d = deadline();
	uint32_t r = release();

	DDRB |= _BV(a->pin);
	PORTB &= ~_BV(a->pin);

	for (;;) {
		if (*a->value > 0) {
			PORTB |= _BV(a->pin);
			d = r += USEC(pgm_read_byte(&factor[*a->value]) << SCALE);
			update(r, d);
		}

		if (*a->value < 255) {
			PORTB &= ~_BV(a->pin);
			d = r += USEC(pgm_read_byte(&factor[(255 - *a->value)]) << SCALE);
			update(r, d);
		}
	}
}
