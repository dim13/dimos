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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bootloader.h"

void
dumphex(struct page *p, int pages, int pagesize)
{
	int n, off, used = 0;

	for (n = 0; n < pages; n++) {
		if (p[n].dirty) {
			++used;
			printf("page %3d (%.4x)", n, n * pagesize);
			for (off = 0; off < pagesize; off++) {
				if (off % 16 == 0)
					printf("\n");
				printf("%3.2x", p[n].data[off]);
			}
			printf("\n");
		}
	}

	printf("used: %3d/%3d (%5.2f%%)\n", used, pages,
	    100 * used / (float)pages);
}

int
main(int argc, char **argv)
{
	struct	page *p;

	if (argc != 2)
		return -1;

	p = rdhex(argv[1], PAGENUM, PAGESIZE);
	assert(p);
	dumphex(p, PAGENUM, PAGESIZE);
	freehex(p, PAGENUM);

	return 0;
}
