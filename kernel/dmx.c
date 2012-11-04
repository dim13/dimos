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
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/cpufunc.h>
#include "kernel.h"
#include "tasks.h"

struct adcarg adcarg;
struct ppmarg ppmarg = { adcarg.value };
struct rgbarg rgbargs = { 0, 0, 0, adcarg.value };
struct pwmarg pwmargs[] = {
	{ &rgbargs.r, PB2 },
	{ &rgbargs.g, PB3 },
	{ &rgbargs.b, PB4 }
};

int
main()
{
	uart_init();
	lcd_init();
	init(nSema);

	exec(heartbeat, NULL, DEFSTACK);
	exec(rgb, &rgbargs, DEFSTACK);
	exec(pwm, &pwmargs[0], DEFSTACK);
	exec(pwm, &pwmargs[1], DEFSTACK);
	exec(pwm, &pwmargs[2], DEFSTACK);
	exec(adc, &adcarg, DEFSTACK);
	exec(clock, NULL, DEFSTACK);
	exec(ctrl, NULL, DEFSTACK);
#if 0
	exec(cmd, &rgbargs, DEFSTACK);
	exec(ppm, &ppmarg, DEFSTACK);
#endif

	IDLE();

	return 0;
}
