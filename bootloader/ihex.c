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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bootloader.h"

int
rdbyte(char **s, int *sum)
{
	int x;

	sscanf(*s, "%2x", &x);

	*s += 2;
	*sum += x;

	return x;
}

struct page *
rdhex(char *fname, int pages, int pagesize)
{
	struct page *p;
	FILE	*fd;
	uint	i, length, addr, addrh, addrl, cksum, page, off;
	char	*s, buf[80];

	fd = fopen(fname, "r");
	if (!fd)
		return NULL;

	p = calloc(pages, sizeof(struct page));

	while ((s = fgets(buf, sizeof(buf), fd)) != NULL) {
		if (*s++ != ':')
			continue;

		cksum = 0;
		length = rdbyte(&s, &cksum);
		addrh = rdbyte(&s, &cksum);
		addrl = rdbyte(&s, &cksum);
		if (rdbyte(&s, &cksum))	/* type, has to be 0 (DATA) */
			continue;

		addr = (addrh << 8) | addrl;
		for (i = 0; i < length; i++) {
			page = (addr + i) / pagesize;
			off = (addr + i) % pagesize;
			if (!p[page].dirty) {
				p[page].data = malloc(pagesize);
				memset(p[page].data, 0xff, pagesize);
			}
			p[page].data[off] = rdbyte(&s, &cksum);
			p[page].dirty = 1;
		}

		rdbyte(&s, &cksum);	/* checksum, last byte */
		if (cksum & 0xff)
			return NULL;
	}

	fclose(fd);

	return p;
}

void
freehex(struct page *p, int pages)
{
	int	i;

	for (i = 0; i < pages; i++)
		if (p[i].dirty)
			free(p[i].data);

	free(p);
}
