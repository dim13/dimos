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

#include <inttypes.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>
#include "kernel.h"
#include "tasks.h"

void
uart_putchar(char c)
{
	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = c;
}

char
uart_getchar(void)
{
	char c;

	loop_until_bit_is_set(UCSRA, RXC);

	if (UCSRA & _BV(FE))
		return -2;		/* EOF */
	if (UCSRA & _BV(DOR))
		return -1;		/* ERR */
	c = UDR;

	switch (c) {
	case '\r':
		c = '\n';
		break;
	case '\n':
		uart_putchar(c);
		break;
	case '\t':
		c = ' ';
		break;
	default:
		break;
	}

	return c;
}

ISR(SIG_UART_RECV)
{
	uint8_t	c = UDR;
	uint8_t *p = 0;

	switch (c) {
	case 'R':	/* reboot */
		wdt_enable(WDTO_15MS);
		break;
	case 'D':	/* dump */
		while (p <= (uint8_t *)RAMEND)
			uart_putchar(*p++);
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
		uart_putchar('?');
		break;
	}
}

ISR(SIG_UART_DATA)
{
	uint8_t r = running();

	UDR = r ? '0' + r : '.';
}

void
init_uart(void)
{
	UCSRB = _BV(RXCIE) | _BV(RXEN) | _BV(TXEN);
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
#if USE_2X
	UCSRA |= _BV(U2X);
#else
	UCSRA &= ~_BV(U2X);
#endif
}
