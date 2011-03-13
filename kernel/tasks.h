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

#ifndef __TASKS_H
#define __TASKS_H

#define ADCCHANNELS 6
#define ADCPRESCALE 32

#if (ADCPRESCALE == 1)
#define ADC_FLAGS 0
#elif (ADCPRESCALE == 2)
#define ADC_FLAGS _BV(ADPS0)
#elif (ADCPRESCALE == 4)
#define ADC_FLAGS _BV(ADPS1)
#elif (ADCPRESCALE == 8)
#define ADC_FLAGS (_BV(ADPS1) | _BV(ADPS0))
#elif (ADCPRESCALE == 16)
#define ADC_FLAGS _BV(ADPS2)
#elif (ADCPRESCALE == 32)
#define ADC_FLAGS (_BV(ADPS2) | _BV(ADPS0))
#elif (ADCPRESCALE == 64)
#define ADC_FLAGS (_BV(ADPS2) | _BV(ADPS1))
#elif (ADCPRESCALE == 128)
#define ADC_FLAGS (_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0))
#else
#warning "invalid ADCPRESCALE value"
#endif

struct rgbarg {
	uint8_t *r, *g, *b;
};

struct pwmarg {
	uint8_t *value;
	uint8_t pin;
};

struct adcarg {
	uint16_t *value;
};

struct lcdarg {
	char *first;
	char *second;
	uint16_t *adc;
};

void	init_uart(void);

void	heartbeat(void *);
void	rgb(void *);
void	cpwm(void *);
void	lcd(void *);
void	adc(void *);

#endif
