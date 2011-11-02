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
#include "kernel.h"
#include "tasks.h"

struct rgbarg rgbargs;

struct pwmarg pwmargs[] = {
	{ &rgbargs.r, PB2, &rgbargs.m },
	{ &rgbargs.g, PB3, &rgbargs.m },
	{ &rgbargs.b, PB4, &rgbargs.m }
};

struct lcdarg lcdarg;

int
main()
{
	init(STACK);
	init_uart();

#if 1
	exec(heartbeat, STACK, 0);	/* stack 48 */
#endif

#if 1
	exec(rgb, STACK + 16, &rgbargs);
	exec(pwm, STACK, &pwmargs[0]);
	exec(pwm, STACK, &pwmargs[1]);
	exec(pwm, STACK, &pwmargs[2]);
#endif

#if 0
	exec(cmd, STACK, &rgbargs);
#endif

#if 1
	exec(lcd, STACK, &lcdarg);
	exec(clock, STACK, &lcdarg);
#endif
	idle();

	return 0;
}
