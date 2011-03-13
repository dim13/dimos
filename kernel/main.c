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

uint8_t	red, green, blue;
uint16_t adcval[ADCCHANNELS];

struct rgbarg rgbargs = { &red, &green, &blue };
struct pwmarg pwmargs[] = {
	{ &red, PB2 },
	{ &green, PB3 },
	{ &blue, PB4 }
};
struct adcarg adcarg = { adcval };
struct lcdarg lcdarg = { 0, 0, adcval };

int
main()
{
	init(STACK);

	init_uart();

	task(heartbeat, STACK, SEC(0), MSEC(10), 0);
	task(rgb, STACK, SEC(0), MSEC(10), &rgbargs);
	task(cpwm, STACK, SEC(0), MSEC(10), &pwmargs[0]);
	task(cpwm, STACK, SEC(0), MSEC(10), &pwmargs[1]);
	task(cpwm, STACK, SEC(0), MSEC(10), &pwmargs[2]);
	task(lcd, STACK, MSEC(40), SEC(1), &lcdarg);
	task(adc, STACK, MSEC(0), MSEC(20), &adcarg);

	for (;;);

	return 0;
}
