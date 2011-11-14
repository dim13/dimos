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
	init(STACK);
	init_uart();

#if 1
	exec(heartbeat, STACK, 0);
#endif

#if 1
	exec(rgb, STACK + 16, &rgbargs);
	exec(pwm, STACK, &pwmargs[0]);
	exec(pwm, STACK, &pwmargs[1]);
	exec(pwm, STACK, &pwmargs[2]);
#endif

#if 1
	exec(adc, STACK, &adcarg);
#endif

#if 1
	exec(lcd, STACK, &lcdarg);
	exec(clock, STACK + 48, &clockarg);
#endif

#if 0
	exec(cmd, STACK, &rgbargs);
#endif

#if 0
	exec(ppm, STACK, &ppmarg);
#endif

	for (;;)
		_NOP();

	return 0;
}
