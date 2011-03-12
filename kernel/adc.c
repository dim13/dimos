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

uint16_t
rdadc(uint8_t ch)
{
	ADMUX &= ~0x07;
	ADMUX |= (ch & 0x07);
	ADCSRA |= _BV(ADSC);
	loop_until_bit_is_set(ADCSRA, ADSC);

	return ADCW;
}

void
adc(void *arg)
{
	struct adcarg *a = (struct adcarg *)arg;
	a->r = release();
	a->d = deadline();

	ADCSRA |= (_BV(ADEN) | _BV(ADFR) | ADC_FLAGS);

	for (;;) {
		*a->value = rdadc(a->channel);
		a->r = a->d += MSEC(20);
		update(a->r, a->d);
	}
}
