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
#include <unistd.h>
#include <string.h>
#include "bootloader.h"

int
transfer(int fd, FILE *dump, int ramend)
{
	int	n;

	put('D', fd);

	printf("reading mem: ");

	for (n = 0; n <= ramend; n++) {
		twiddle();
		fputc(get(fd), dump);
	}
	printf("\n");

	return 0;
}

int
main(int argc, char **argv)
{
	char	*dev = "/dev/ttyU0";
	int	c, fd;
	FILE	*dump;

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

	dump = fopen("memdump", "w");
	fd = open_tty(dev);
	transfer(fd, dump, RAMEND);
	close(fd);
	fclose(dump);

	return 0;
}
