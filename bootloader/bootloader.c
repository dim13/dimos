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

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "bootloader.h"

int
transfer(int fd, struct page *p, int pages, int pagesize)
{
	int	n, off, maxerr = 3;
	unsigned char sum;

	fprintf(stderr, "waiting for bootloader ...");
	put('-', fd);
	put('\r', fd);
	put('\n', fd);
	while (get(fd) != '+')
		;

	fprintf(stderr, "\nwriting pages:");

	for (n = 0; n < pages; n++) {
		if (!p[n].dirty)
			continue;
			
		put('@', fd);
		put(n, fd);
		sum = n;
		for (off = 0; off < pagesize; off++) {
			put(p[n].data[off], fd);
			sum += p[n].data[off];
		}
		put(sum, fd);
		switch (get(fd)) {
		case '.':
			if (!(n % 10))
				fprintf(stderr, " %d", n);
			fprintf(stderr, ".");
			maxerr = 0;
			break;	/* success, next page */
		case '!':
			n--;	/* error stay on the same page */
			fprintf(stderr, "E");
			if (!maxerr--)
				goto fubar;
			break;
		default:
			if (!maxerr--)
				goto fubar;
			fprintf(stderr, "\ngarbage on the line, retry\n");
			return transfer(fd, p, pages, pagesize);
		}
	}

fubar:

	fprintf(stderr, "\nrebooting\n");
	put('-', fd);

	return 0;
}

int
main(int argc, char **argv)
{
	struct	page *p;
	char	*dev = "/dev/ttyU0";
	int	c, fd;

	while ((c = getopt(argc, argv, "f:")) != -1)
		switch (c) {
		case 'f':
			dev = strdup(optarg);
			break;
		default:
			break;
		}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		return -1;

	p = rdhex(argv[0], PAGENUM, PAGESIZE);

	if (!p)
		errx(1, "error reading file %s", argv[0]);

	/* protect firmware */
	for (c = GUARDPAGE; c < PAGENUM; c++)
		if (p[c].dirty)
			errx(1, "firmware protection: programm to big");

	fd = open_tty(dev);
	if (fd == -1) {
		perror(dev);
		return -1;
	}
	transfer(fd, p, PAGENUM, PAGESIZE);
	close(fd);

	freehex(p, PAGENUM);

	return 0;
}
