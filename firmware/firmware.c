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

#ifndef BAUD
#define BAUD 9600
#endif

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/wdt.h>
#include <util/setbaud.h>	/* depends on BAUD & F_CPU env vars */

#define TIMEOUT	(F_CPU >> 4)	/* ~ 1 sec */
#define PUTCH(c) do { \
	loop_until_bit_is_set(UCSR0A, UDRE0); UDR0 = (c); \
} while (0)

union {
	uint16_t word;
	uint8_t	byte[2];
} data;

enum { INIT, PAGE, DATA, CKSUM };

int
main(void)
{
	uint32_t c = 0;
	uint16_t off = 0;
	uint8_t n = 0;
	uint8_t ch = 0;
	uint8_t sum = 0;
	uint8_t state = INIT;

	MCUSR = 0;
	wdt_disable();

	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	#if USE_2X
	UCSR0A |= _BV(U2X0);
	#else
	UCSR0A &= ~_BV(U2X0);
	#endif

	PUTCH('+');		/* say hallo */
	for (;;) {
		for (c = 0; bit_is_clear(UCSR0A, RXC0); c++)
			if (c > TIMEOUT)
				goto reboot;
		ch = UDR0;	/* GETCH */

		switch (state) {
		case INIT:
			switch (ch) {
			case '@':
				state = PAGE;
				break;
			case '-':
				goto reboot;
			default:
				break;
			}
			break;
		case PAGE:
			n = 0;
			sum = ch;
			off = (uint16_t)ch * SPM_PAGESIZE;
			state = DATA;
			break;
		case DATA:
			sum += ch;
			data.byte[n % 2] = ch;
			if (n % 2)
				boot_page_fill(off + n - 1, data.word);
			if (++n == SPM_PAGESIZE)
				state = CKSUM;
			break;
		case CKSUM:
			if (sum == ch) {
				boot_page_erase(off);
				boot_spm_busy_wait();
				boot_page_write(off);
				boot_spm_busy_wait();
				PUTCH('.');	/* confirm */
			} else
				PUTCH('!');	/* flag error */
			state = INIT;
			break;
		}
	}

reboot:
	boot_rww_enable();

	((void(*)(void))0)();
	/* NOTREACHED */

	return 0;
}
