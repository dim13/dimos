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

/* globals */
uint8_t	red, green, blue;
struct rgbarg rgbargs = { &red, &green, &blue };
struct pwmarg pwmargs[] = {
	{ &red, PB2 },
	{ &green, PB3 },
	{ &blue, PB4 }
};
struct lcdarg lcdarg;
struct clockarg clockarg;
struct ctrlarg ctrlarg = { &lcdarg, &clockarg };

int
main()
{
	init(STACK);
	init_uart();

	semaphore(0, 1);

	exec(heartbeat, STACK, 0);	// 48

#if 1
	exec(rgb, STACK + 16, &rgbargs);
	exec(pwm, STACK, &pwmargs[0]);
	exec(pwm, STACK, &pwmargs[1]);
	exec(pwm, STACK, &pwmargs[2]);
#if 0
	exec(cmd, STACK, &rgbargs);
#endif
#if 1
	exec(lcd, STACK, &lcdarg);
	exec(clock, STACK, &clockarg);
	exec(ctrl, STACK + 8, &ctrlarg);
#endif
#endif

	for (;;);	/* idle task */

	return 0;
}
