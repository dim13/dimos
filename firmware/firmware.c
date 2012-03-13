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

#include <avr/io.h>
#include <avr/boot.h>
#include <util/setbaud.h>	/* depends on BAUD & F_CPU env vars */

#define TIMEOUT (F_CPU / PRESCALE)	/* 1 sec */
#define PUTCH(c) do { loop_until_bit_is_set(UCSRA, UDRE); UDR = (c); } while (0)

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
	uint8_t ch = 0;
	uint8_t n = 0;
	uint8_t sum = 0;
	uint8_t state = INIT;

	UCSRB = _BV(RXEN) | _BV(TXEN);
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	UCSRA &= ~_BV(U2X);

	PUTCH('+');		/* say hallo */
	for (;;) {
		for (c = 0; bit_is_clear(UCSRA, RXC); c++)
			if (c > TIMEOUT)
				goto reboot;
		ch = UDR;	/* GETCH */

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
