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
#include "kernel.h"
#include "tasks.h"

#define	ON	PORTB |= _BV(PB1)
#define	OFF	PORTB &= ~_BV(PB1)

#if 0
void
ppm(void *arg)
{
	struct ppmarg *a = (struct ppmarg *)arg;
	uint32_t r = release();
	uint32_t d = deadline();
	uint32_t t;
	uint32_t s;
	uint8_t i;

	DDRB |= _BV(DDB1);
	PORTB &= ~_BV(PB1);	/* high is low */

	/* frame length 22.5ms, channel 0.7-1.7ms, stop 0.3 ms */

	for (;;) {
		s = 0;

			t = (uint32_t)a->value[i] * MSEC(1) / 0x3ff;
			s += MSEC(1) + t;

			/* channel frame 0.7..1.7ms high */
			OFF;
			r = d += MSEC(1) + t;
			update(r, d);

			/* stop frame 0.3ms low */
			ON;
			r = d += MSEC(25) - t - MSEC(1);
			update(r, d);
	}
}
#endif

void
ppm(void *arg)
{
	struct ppmarg *a = (struct ppmarg *)arg;
	uint32_t r = release();
	uint32_t d = deadline();
	uint32_t t;
	uint8_t i;

	DDRB |= _BV(DDB1);
	PORTB &= ~_BV(PB1);	/* high is low */
	
	/* frame length 22.5ms, channel 0.7-1.7ms, stop 0.3 ms */

	for (;;) {
		/* start frame 0.3ms low */
		OFF;
		r = d += USEC(300);
		update(r, d);

		wait(0);
		for (i = 0, t = 0; i < ADCCHANNELS; i++)
			t += a->value[i];

		/* sync frame */
		ON;
		//r = d += USEC(22500) - MSEC(t) / 0x3ff - MSEC(ADCCHANNELS) - USEC(300);
		r = d += MSEC(20) - MSEC(t) / 0x3ff - MSEC(ADCCHANNELS) - USEC(300);
		update(r, d);

		for (i = 0; i < ADCCHANNELS; i++) {

			/* start frame 0.3ms low */
			OFF;
			r = d += USEC(300);
			update(r, d);

			/* channel frame 0.7..1.7ms high */
			ON;
			r = d += USEC(700) + MSEC(a->value[i]) / 0x3ff;
			update(r, d);
		}
		signal(0);

	}
}
