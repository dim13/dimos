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

/* 1/sqrt(2) timing scala in usec */
uint16_t factor[] PROGMEM = {
    30,    30,    31,    31,    32,    32,    33,    34,
    34,    35,    35,    36,    37,    37,    38,    39,
    39,    40,    41,    41,    42,    43,    44,    44,
    45,    46,    47,    48,    48,    49,    50,    51,
    52,    53,    54,    55,    56,    57,    58,    59,
    60,    61,    62,    63,    64,    65,    66,    68,
    69,    70,    71,    72,    74,    75,    76,    78,
    79,    80,    82,    83,    85,    86,    88,    89,
    91,    92,    94,    96,    97,    99,   101,   103,
   104,   106,   108,   110,   112,   114,   116,   118,
   120,   122,   124,   126,   129,   131,   133,   136,
   138,   140,   143,   145,   148,   150,   153,   156,
   158,   161,   164,   167,   170,   173,   176,   179,
   182,   185,   189,   192,   195,   199,   202,   206,
   209,   213,   217,   220,   224,   228,   232,   236,
   240,   245,   249,   253,   258,   262,   267,   272,
   276,   281,   286,   291,   296,   301,   307,   312,
   317,   323,   329,   334,   340,   346,   352,   358,
   365,   371,   378,   384,   391,   398,   405,   412,
   419,   426,   434,   441,   449,   457,   465,   473,
   481,   490,   498,   507,   516,   525,   534,   544,
   553,   563,   573,   583,   593,   603,   614,   625,
   635,   647,   658,   669,   681,   693,   705,   717,
   730,   743,   756,   769,   782,   796,   810,   824,
   839,   853,   868,   883,   899,   915,   931,   947,
   963,   980,   997,  1015,  1033,  1051,  1069,  1088,
  1107,  1126,  1146,  1166,  1186,  1207,  1228,  1250,
  1271,  1294,  1316,  1339,  1363,  1386,  1411,  1435,
  1460,  1486,  1512,  1538,  1565,  1593,  1621,  1649,
  1678,  1707,  1737,  1767,  1798,  1830,  1862,  1894,
  1927,  1961,  1995,  2030,  2066,  2102,  2138,  2176,
  2214,  2253,  2292,  2332,  2373,  2414,  2457,  2500,
};

struct bb {
	uint8_t color, r, g, b;
} bb[] PROGMEM = {
	{ 10, 0xff, 0x38, 0x00 },
	{ 11, 0xff, 0x47, 0x00 },
	{ 12, 0xff, 0x53, 0x00 },
	{ 13, 0xff, 0x5d, 0x00 },
	{ 14, 0xff, 0x65, 0x00 },
	{ 15, 0xff, 0x6d, 0x00 },
	{ 16, 0xff, 0x73, 0x00 },
	{ 17, 0xff, 0x79, 0x00 },
	{ 18, 0xff, 0x7e, 0x00 },
	{ 19, 0xff, 0x83, 0x00 },
	{ 20, 0xff, 0x89, 0x12 },
	{ 21, 0xff, 0x8e, 0x21 },
	{ 22, 0xff, 0x93, 0x2c },
	{ 23, 0xff, 0x98, 0x36 },
	{ 24, 0xff, 0x9d, 0x3f },
	{ 25, 0xff, 0xa1, 0x48 },
	{ 26, 0xff, 0xa5, 0x4f },
	{ 27, 0xff, 0xa9, 0x57 },
	{ 28, 0xff, 0xad, 0x5e },
	{ 29, 0xff, 0xb1, 0x65 },
	{ 30, 0xff, 0xb4, 0x6b },
	{ 31, 0xff, 0xb8, 0x72 },
	{ 32, 0xff, 0xbb, 0x78 },
	{ 33, 0xff, 0xbe, 0x7e },
	{ 34, 0xff, 0xc1, 0x84 },
	{ 35, 0xff, 0xc4, 0x89 },
	{ 36, 0xff, 0xc7, 0x8f },
	{ 37, 0xff, 0xc9, 0x94 },
	{ 38, 0xff, 0xcc, 0x99 },
	{ 39, 0xff, 0xce, 0x9f },
	{ 40, 0xff, 0xd1, 0xa3 },
	{ 41, 0xff, 0xd3, 0xa8 },
	{ 42, 0xff, 0xd5, 0xad },
	{ 43, 0xff, 0xd7, 0xb1 },
	{ 44, 0xff, 0xd9, 0xb6 },
	{ 45, 0xff, 0xdb, 0xba },
	{ 46, 0xff, 0xdd, 0xbe },
	{ 47, 0xff, 0xdf, 0xc2 },
	{ 48, 0xff, 0xe1, 0xc6 },
	{ 49, 0xff, 0xe3, 0xca },
	{ 50, 0xff, 0xe4, 0xce },
	{ 51, 0xff, 0xe6, 0xd2 },
	{ 52, 0xff, 0xe8, 0xd5 },
	{ 53, 0xff, 0xe9, 0xd9 },
	{ 54, 0xff, 0xeb, 0xdc },
	{ 55, 0xff, 0xec, 0xe0 },
	{ 56, 0xff, 0xee, 0xe3 },
	{ 57, 0xff, 0xef, 0xe6 },
	{ 58, 0xff, 0xf0, 0xe9 },
	{ 59, 0xff, 0xf2, 0xec },
	{ 60, 0xff, 0xf3, 0xef },
	{ 61, 0xff, 0xf4, 0xf2 },
	{ 62, 0xff, 0xf5, 0xf5 },
	{ 63, 0xff, 0xf6, 0xf8 },
	{ 64, 0xff, 0xf8, 0xfb },
	{ 65, 0xff, 0xf9, 0xfd },
	{ 66, 0xfe, 0xf9, 0xff },
	{ 67, 0xfc, 0xf7, 0xff },
	{ 68, 0xf9, 0xf6, 0xff },
	{ 69, 0xf7, 0xf5, 0xff },
	{ 70, 0xf5, 0xf3, 0xff },
	{ 71, 0xf3, 0xf2, 0xff },
	{ 72, 0xf0, 0xf1, 0xff },
	{ 73, 0xef, 0xf0, 0xff },
	{ 74, 0xed, 0xef, 0xff },
	{ 75, 0xeb, 0xee, 0xff },
	{ 76, 0xe9, 0xed, 0xff },
	{ 77, 0xe7, 0xec, 0xff },
	{ 78, 0xe6, 0xeb, 0xff },
	{ 79, 0xe4, 0xea, 0xff },
	{ 80, 0xe3, 0xe9, 0xff },
	{ 81, 0xe1, 0xe8, 0xff },
	{ 82, 0xe0, 0xe7, 0xff },
	{ 83, 0xde, 0xe6, 0xff },
	{ 84, 0xdd, 0xe6, 0xff },
	{ 85, 0xdc, 0xe5, 0xff },
	{ 86, 0xda, 0xe4, 0xff },
	{ 87, 0xd9, 0xe3, 0xff },
	{ 88, 0xd8, 0xe3, 0xff },
	{ 89, 0xd7, 0xe2, 0xff },
	{ 90, 0xd6, 0xe1, 0xff },
};

void
rgb(void *arg)
{
	struct rgbarg *a = (struct rgbarg *)arg;
	uint16_t i = 0;
	uint8_t v;

	update(0, MSEC(370));

	for (;;) {
#if 1
		i = (i + 1) % 360;
		#if 0
		v = i % 120;
		v = (v < 60) ? 255 - 2 * v : 15 + 2 * v;
		#else
		v = 255;
		#endif

		hsv(a->r, a->g, a->b, i, 255, v);
		sleep(SOFT, MSEC(83));
#else
		i = (i + 1) % 162;
		if (i > 80)
			v = 161 - i;
		else
			v = i;
		*a->r = pgm_read_byte(&bb[v].r);
		*a->g = pgm_read_byte(&bb[v].g);
		*a->b = pgm_read_byte(&bb[v].b);

		sleep(SOFT, MSEC(370));
#endif
	}
}

void
pwm(void *arg)
{
	struct pwmarg *a = (struct pwmarg *)arg;
	uint16_t on, off, maxval;;
	uint32_t n = now();

	DDRB |= _BV(a->pin);
	PORTB &= ~_BV(a->pin);
	maxval = pgm_read_word(&factor[255]);

	update(n, n + USEC(maxval));

	for (;;) {
		on = pgm_read_word(&factor[*a->value]);
		off = maxval - on;

		if (*a->value > 0) {
			PORTB |= _BV(a->pin);
			sleep(HARD, USEC(on));
		}

		if (*a->value < 255) {
			PORTB &= ~_BV(a->pin);
			sleep(HARD, USEC(off));
		}
	}
}
