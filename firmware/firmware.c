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
#include <util/setbaud.h>	/* depends on BAUD & F_CPU env vars */
#include <avr/boot.h>

#define TIMEOUT (F_CPU >> 3)	/* ca. 2 sec */

union {
	uint16_t word;
	uint8_t	byte[2];
} data;

void
reboot(void)
{
	boot_rww_enable();

	((void(*)(void))0)();	/* jump to app */
}

void
putch(char c)
{
	loop_until_bit_is_set(UCSRA, UDRE);

	UDR = c;
}

uint8_t
getch(void)
{
	uint32_t counter = 0;

	do {
		if (++counter > TIMEOUT)
			reboot();
	} while (bit_is_clear(UCSRA, RXC));

	return UDR;
}

enum { INIT, PAGE, DATA, CKSUM };

int
main(void)
{

	uint16_t off = 0;
	uint8_t ch = 0;
	uint8_t n = 0;
	uint8_t sum = 0;
	uint8_t state = INIT;

	UCSRB = _BV(RXEN) | _BV(TXEN);
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	UCSRA &= ~_BV(U2X);

	putch('+');		/* say hallo */
	for (;;) {
		ch = getch();
		switch (state) {
		case INIT:
			switch (ch) {
			case '@':
				state = PAGE;
				break;
			case '-':
				reboot();
				break;
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
				putch('.');	/* confirm */
			} else
				putch('!');	/* flag error */
			state = INIT;
			break;
		}
	}

	return 0;
}
