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

#ifndef TASKS
#warning "TASKS not set, fallback to default"
#define TASKS 8
#endif

#ifndef SEMAPHORES
#warning "SEMAPHORES not set, fallback to default"
#define SEMAPHORES 8
#endif

#ifndef STACK
#warning "STACK not set, fallback to default"
#define STACK 64
#endif

#ifndef F_CPU
#warning "F_CPU not set, fallback to default"
#define F_CPU 1000000
#endif

#ifndef PRESCALE
#warning "PRESCALE not set, fallback to default"
#define PRESCALE 1
#endif

#if (PRESCALE == 1)
#define TIMER_FLAGS _BV(CS10)
#elif (PRESCALE == 8)
#define TIMER_FLAGS _BV(CS11)
#elif (PRESCALE == 64)
#define TIMER_FLAGS (_BV(CS11) | _BV(CS10))
#elif (PRESCALE == 256)
#define TIMER_FLAGS _BV(CS12)
#elif (PRESCALE == 1024)
#define TIMER_FLAGS (_BV(CS12) | _BV(CS10))
#else
#warning "invalid PRESCALE value"
#endif

#define SEC(T)	((uint32_t)(T) * (F_CPU / PRESCALE))
#define MSEC(T)	((uint32_t)(T) * ((F_CPU / 1000) / PRESCALE))
#define USEC(T)	((uint32_t)(T) * ((F_CPU / 1000000) / PRESCALE))

void init(int idlestack);
void task(void (*fun)(void *), uint16_t stack, void *args);
void semaphore(uint8_t semnbr, uint8_t initVal);

void wait(uint8_t semnbr);
void signal(uint8_t semnbr);
void suspend(void);

void update(uint32_t release, uint32_t deadline);
enum { SOFT, HARD };
void sleep(uint8_t, uint32_t);

uint32_t now(void);
uint32_t release(void);
uint32_t deadline(void);
uint8_t running(void);

#endif
