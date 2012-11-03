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
#include <stdio.h>
#include "kernel.h"
#include "tasks.h"

void
ctrl(void *arg)
{
	int c;
	uint8_t	*p;

	for (;;) {
		switch ((c = getchar())) {
		case 'R':
		case 'r':
		case '-':
			reboot();
			break;
		case 'D':
		case 'd':
			for (p = (uint8_t *)0; p <= (uint8_t *)RAMEND; p++)
				putchar(*p);
			break;	
		case 'N':
		case 'n':
			printf("%8lx\n", now());
			break;
		case -2:
			puts("EOF");
			break;
		case -1:
			puts("ERR");
			break;
		default:
			puts("?");
			break;
		}
	}
}
