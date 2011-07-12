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
clock(void *arg)
{
	struct clockarg *a = arg;

	a->s = a->m = a->h = a->d = 0;

	update(0, SEC(1));

	for (;;) {
		a->s += 1;
		if (a->s == 60) {
			a->s = 0;
			a->m += 1;
		}
		if (a->m == 60) {
			a->m = 0;
			a->h += 1;
		}
		if (a->h == 24) {
			a->h = 0;
			a->d += 1;
		}

		sleep(HARD, SEC(1));
	}
}
