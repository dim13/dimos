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
#include <stdlib.h>
#include "kernel.h"
#include "tasks.h"

void
uputs(char *s)
{
	while (*s)
		uart_putchar(*s++);
	uart_putchar('\r');
	uart_putchar('\n');
}

void
cmd(void *arg)
{
	struct rgbarg *a = arg;
	uint32_t d = deadline();
	uint32_t r = release();
	char c;
	int val;
	char buf[10], *s;

	init_uart();

	s = buf;

	for (;;) {
		c = uart_getchar();
		if (c > 0) {
			if (c >= '0' && c <= '9')
				*s++ = c;
			if (c == '\n' || c == '\r' || s - buf >= 9) {
				*s = '\0';
				s = buf;
				uputs(buf);
				val = atoi(buf);
				if (val >= 0 && val < 256) {
					*a->r = *a->g = *a->b = val;
					uputs("OK");
				} else {
					uputs("ERR");
				}
			}
		}
		
		d = r += MSEC(10);
		update(r, d);
	}
}
