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

#define ADCCHANNELS 4		/* max 6 */
#define ADCPRESCALE 128		/* 50-200 kHz for max resolution */

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
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t m;
	uint16_t *v;
};

struct pwmarg {
	uint8_t *value;
	uint8_t pin;
	uint8_t *mval;
};

struct adcarg {
	uint16_t *value;
	uint16_t *max;
	uint16_t *min;
};

struct lcdarg {
	char first[18];
	char second[18];
	uint8_t x, y;
};

struct clockarg {
	struct lcdarg *lcd;
	struct adcarg *adc;
};

struct ppmarg {
	uint16_t *value;
};

void	init_uart(void);
int	uart_getchar(void);
int	uart_putchar(char);

void	heartbeat(void *);
void	rgb(void *);
void	pwm(void *);
void	lcd(void *);
void	adc(void *);
void	ppm(void *);
void	cmd(void *);
void	clock(void *);

#endif
