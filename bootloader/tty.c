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
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include "bootloader.h"

void
put(unsigned char c, int fd)
{
#if DEBUG
	fprintf(stderr, "-> 0x%.2x\n", c);
#endif
	while (write(fd, &c, 1) == -1)
		;
}

unsigned char
get(int fd)
{
	unsigned char	c;

	while (read(fd, &c, 1) == -1)
		;
#if DEBUG
	fprintf(stderr, "<- 0x%.2x\n", c);
#endif
	return c;
}

void
twiddle(void)
{
	static int pos;

	putchar("|/-\\"[pos++ & 3]);
	putchar('\b');
}


int
open_tty(char *dev)
{
	struct	termios tio;
	int	fd;

	fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
		return -1;

	fprintf(stderr, "open done, trying to set raw\n");

	bzero(&tio, sizeof(tio));
	tio.c_cflag = CS8 | CREAD | CLOCAL;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 10;
	cfsetspeed(&tio, B9600);
	cfmakeraw(&tio);
	if (tcsetattr(fd, TCSANOW, &tio) < 0)
		fprintf(stderr, "cannot set raw mode\n");

	usleep(500);

	return fd;
}
