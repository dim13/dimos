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
#include <avr/sleep.h>
#include "kernel.h"
#include "tasks.h"

struct adcarg adcarg;
struct ppmarg ppmarg = { adcarg.value };
struct rgbarg rgbargs = { 0, 0, 0, adcarg.value };
struct pwmarg pwmargs[] = {
	{ &rgbargs.r, PB2, 1 },
	{ &rgbargs.g, PB3, 2 },
	{ &rgbargs.b, PB4, 3 }
};

int
main()
{
	uart_init();
	lcd_init();
	init(48);

	exec(heartbeat, NULL, 48);
	exec(rgb, &rgbargs, 72);
	exec(pwm, &pwmargs[0], 56);
	exec(pwm, &pwmargs[1], 56);
	exec(pwm, &pwmargs[2], 56);
	exec(adc, &adcarg, 96);
	exec(clock, NULL, 96);
#if 0
	exec(cmd, &rgbargs, 48);
	exec(ppm, &ppmarg, 48);
#endif

	for (;;)
		sleep_mode();

	return 0;
}
