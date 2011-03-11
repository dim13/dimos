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
reboot()
{
	boot_rww_enable();
	((void(*)(void))0)();	/* jump to app */
}

void
putch(char c)
{
	if (c == '\n')
		putch('\r');	/* be unix polite */

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

int
main(void)
{
	uint8_t i;
	uint16_t off;

	UCSRB = _BV(RXEN) | _BV(TXEN);
	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	UCSRA &= ~_BV(U2X);

	putch('+');		/* say hallo */

	if (getch() != 'P')	/* wait a while for program request */
		reboot();
	putch('p');		/* confirm */

	for (;;) {
		switch (getch()) {
		case 'D':	/* data request */
			off = (uint16_t)getch() * SPM_PAGESIZE;
			for (i = 0; i < SPM_PAGESIZE; i += 2) {
				data.byte[0] = getch();
				data.byte[1] = getch();
				boot_page_fill(off + i, data.word);
			}

			boot_page_erase(off);
			boot_spm_busy_wait();

			boot_page_write(off);
			boot_spm_busy_wait();

			putch('d');	/* confirm */
			break;
		case 'R':	/* reboot request */
			reboot();
			break;
		default:
			break;
		}
	}

	return 0;
}
