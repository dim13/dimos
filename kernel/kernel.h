/* $Id$ */
/*
 * Copyright (c) 2010 Dimitri Sokolyuk <demon@dim13.org>
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

#ifndef __KERNEL_H
#define __KERNEL_H

#define MINSTACK 0x3f
#define DEFSTACK 0x7f
#define BIGSTACK 0xff

#ifndef F_CPU
#warning F_CPU not set, fallback to default: 16MHz
#define F_CPU 16000000
#endif

#ifndef PRESCALE
#define PRESCALE 1
#endif

#if (PRESCALE == 1)
#define TIMER_FLAGS (_BV(CS10))
#elif (PRESCALE == 8)
#define TIMER_FLAGS (_BV(CS11))
#elif (PRESCALE == 64)
#define TIMER_FLAGS (_BV(CS11) | _BV(CS10))
#elif (PRESCALE == 256)
#define TIMER_FLAGS (_BV(CS12))
#elif (PRESCALE == 1024)
#define TIMER_FLAGS (_BV(CS12) | _BV(CS10))
#else
#error invalid PRESCALE value
#endif

#define Hz	(F_CPU / PRESCALE)
#define kHz	(Hz / 1000)
#define MHz	(kHz / 1000)

#if (!MHz)
#error MHz value too small, adjust PRESCALE and/or F_CPU
#endif

#if (!kHz)
#error kHz value too small, adjust PRESCALE and/or F_CPU
#endif

#if (!Hz)
#error Hz value too small, adjust PRESCALE and/or F_CPU
#endif

#define USECMAX	(INT32_MAX / MHz)
#define MSECMAX	(INT32_MAX / kHz)
#define SECMAX	(INT32_MAX / Hz)

#define USEC(T)	((uint32_t)(T) * MHz)
#define MSEC(T)	((uint32_t)(T) * kHz)
#define SEC(T)	((uint32_t)(T) * Hz)

enum Prio { High, Mid, Low, Idle, nPrio };

/* __BEGIN_DECLS */

void init(uint8_t sema, uint8_t stack);
void exec(void (*fun)(void *), void *args, uint8_t stack);

void wait(uint8_t chan);
void signal(uint8_t chan);

void suspend(void);
void sleep(uint32_t sec, uint32_t usec);
void yield(void);

uint32_t now(void);
uint8_t running(void);

void reboot(void);
void idle(void);

/* __END_DECLS */

#endif
