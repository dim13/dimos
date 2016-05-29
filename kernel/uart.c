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
#define BAUD 9600
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>		/* depends on BAUD and F_CPU */

#include <stdint.h>
#include <stdio.h>

#include "kernel.h"
#include "tasks.h"

void
uart_init(void)
{
	FILE *uart_stream;

	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
#if USE_U2X
	UCSR0A |= _BV(U2X0);
#else
	UCSR0A &= ~_BV(U2X0);
#endif

	uart_stream = fdevopen(uart_putchar, uart_getchar);
	stdin = uart_stream;
	stdout = uart_stream;
}

int
uart_putchar(char c, FILE *fd)
{
	if (c == '\n')
		uart_putchar('\r', fd);
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;

	return 0;
}

int
uart_getchar(FILE *fd)
{
	char c;

	loop_until_bit_is_set(UCSR0A, RXC0);
	c = UDR0;

	if (bit_is_set(UCSR0A, FE0))
		return -2;		/* EOF */
	if (bit_is_set(UCSR0A, DOR0))
		return -1;		/* ERR */

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
