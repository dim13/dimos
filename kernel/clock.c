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
#include <stdio.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include "kernel.h"
#include "tasks.h"

void
clock(void *arg)
{
	uint8_t d, h, m, s, ds;

	d = h = m = s = ds = 0;

	for (;;) {
		++ds;
		if (ds == 10) { ds = 0; ++s; }
		if (s == 60) { s = 0; ++m; }
		if (m == 60) { m = 0; ++h; }
		if (h == 24) { h = 0; ++d; }

		wait(0);
		fprintf(stderr, "\r%4d:%.2d:%.2d:%.2d.%1d ", d, h, m, s, ds);
		signal();

		sleep(SEC1(1));
	}
}
