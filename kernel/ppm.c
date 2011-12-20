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
#include <avr/io.h>
#include <avr/interrupt.h>
#include "kernel.h"
#include "tasks.h"

#define	ON	do { PORTB |=  _BV(PB1); } while (0)
#define	OFF	do { PORTB &= ~_BV(PB1); } while (0)
#define	SWITCH	do { PORTB ^= _BV(PB1); } while (0)

#define ADCMAX	0x3ff			/* 10 bit */
#define DL	SEC4(1)			/* 0.1ms */
#define FRAME	SEC2(2)			/* 20ms */
#define DELIM	SEC4(3)			/* 0.3ms */
#define SIGMIN	SEC4(7)			/* 0.7ms */

void
ppm(void *arg)
{
	struct ppmarg *a = (struct ppmarg *)arg;
	uint32_t t, n, v;
	uint8_t i;

	DDRB |= _BV(DDB1);
	OFF;

	/* frame length 20ms, channel 0.7-1.7ms, stop 0.3 ms */
	for (;;) {
		t = FRAME;

		for (i = 0; i < ADCCHANNELS; i++) {
			cli();
			v = SEC3(a->value[i]);
			sei();
			
			n = SIGMIN + v / ADCMAX;
			t -= n + DELIM;

			/* start frame 0.3ms low */
			SWITCH;
			sleep(DELIM);

			/* channel frame 0.7..1.7ms high */
			SWITCH;
			sleep(n);
		}

		/* sync frame */

		SWITCH;
		sleep(DELIM);

		SWITCH;
		sleep(t - DELIM);
	}
}
