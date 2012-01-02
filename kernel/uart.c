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

#ifndef BAUD
#warning "BAUD not set, fallback to default"
#define BAUD	9600
#endif

#define USE_RXCIE

#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>
#include "kernel.h"
#include "tasks.h"

FILE uart_stream = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

#ifdef USE_RXCIE
ISR(SIG_UART_RECV)
{
	uint8_t	c, *p;

	switch ((c = UDR)) {
	case 'Z':	/* zero */
		for (p = (uint8_t *)RAMSTART; p <= (uint8_t *)RAMEND; p++)
			*p = 'A';
		/* FALLTHROUGH */
	case 'R':	/* reboot */
	case '-':	/* reboot */
		wdt_enable(WDTO_15MS);
		break;
	case 'D':	/* dump */
		for (p = (uint8_t *)0; p <= (uint8_t *)RAMEND; p++)
			uart_putchar(*p, NULL);
		break;
	case 'T':
		UCSRB |= _BV(UDRIE);
		break;
	case 't':
		UCSRB &= ~_BV(UDRIE);
		break;
	case '\r':
	case '\n':
		break;
	default:
		uart_putchar('?', NULL);
		break;
	}
}

ISR(SIG_UART_DATA)
{
	uint8_t r = running();

	UDR = r ? '0' + r : '.';
}
#endif

void
uart_init(void)
{
	UCSRB = _BV(RXEN) | _BV(TXEN);
#ifdef USE_RXCIE
	UCSRB |= _BV(RXCIE);
#endif
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	UCSRA &= ~_BV(U2X);

	stdin = &uart_stream;
	stdout = &uart_stream;
}

int
uart_putchar(char c, FILE *fd)
{
	if (c == '\n')
		uart_putchar('\r', fd);
	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = c;

	return 0;
}

int
uart_getchar(FILE *fd)
{
	char c;

	loop_until_bit_is_set(UCSRA, RXC);

	if (bit_is_set(UCSRA, FE))
		return -2;		/* EOF */
	if (bit_is_set(UCSRA, DOR))
		return -1;		/* ERR */

	c = UDR;
	uart_putchar(c, fd);		/* ECHO */

	switch (c) {
	case '\r':
		c = '\n';
		break;
	case '\t':
		c = ' ';
		break;
	default:
		break;
	}

	return c;
}
