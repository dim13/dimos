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
#include <string.h>
#include <avr/io.h>
#include <avr/cpufunc.h>
#include "kernel.h"
#include "tasks.h"

struct adcarg adcarg;

struct ppmarg ppmarg = { adcarg.value };

struct lcdarg lcdarg;

struct clockarg clockarg = { &lcdarg, &adcarg };

struct rgbarg rgbargs = { 0, 0, 0, &adcarg.value[0] };

struct pwmarg pwmargs[] = {
	{ &rgbargs.r, PB2 },
	{ &rgbargs.g, PB3 },
	{ &rgbargs.b, PB4 }
};

int
main()
{
	init(STACK - 6);				/* 42 */
	init_uart();

#define LOW	0
#define MID	0
#define HIGH	0

#if 1
	exec(heartbeat, NULL, STACK - 10, LOW);		/* 38 */
#endif

#if 1
	exec(rgb, &rgbargs, STACK + 24, MID);		/* 72 */
	exec(pwm, &pwmargs[0], STACK, HIGH);		/* 48 */
	exec(pwm, &pwmargs[1], STACK, HIGH);		/* 48 */
	exec(pwm, &pwmargs[2], STACK, HIGH);		/* 48 */
	exec(adc, &adcarg, STACK - 6, LOW);		/* 42 */
#endif

#if 1
	exec(lcd, &lcdarg, STACK, LOW);			/* 48 */
	exec(clock, &clockarg, STACK + 24, LOW);	/* 72 */
#endif

#if 0
	exec(cmd, &rgbargs, STACK, LOW);		/* 48 */
#endif

#if 0
	exec(ppm, &ppmarg, STACK, LOW);			/* 48 */
#endif

	for (;;)
		_NOP();

	return 0;
}
