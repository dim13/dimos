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
/*
 * based on TinyRealTime by Dan Henriksson and Anton Cervin
 * http://www.control.lth.se/Publication/hen+04t.html
 */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "kernel.h"
#include "stack.h"

enum { TERMINATED, READYQ, TIMEQ, WAITQOFFSET };

#define LO8(x)			((uint8_t)((uint16_t)(x)))
#define HI8(x)			((uint8_t)((uint16_t)(x) >> 8))
#define SCHEDULE		TIMER1_COMPA_vect
#define DISTANCE(from, to)	((int32_t)((to) - (from)))
#define EPOCH			0x3FFFFFFFUL			/* XXX */
#define EPS			(LATENCY / PRESCALE + 1)	/* XXX */
#define NOW(hi, lo)		(((uint32_t)(hi) << 0x10) | (lo))

struct task {
	uint8_t spl;
	uint8_t sph;
	uint32_t release;
	uint32_t deadline;
	uint8_t state;
};

struct kernel {
	struct task *running;
	struct task *prev;
	struct task *idle;
	struct task *first;
	struct task *last;
	struct task task[TASKS + 1];
	uint8_t semaphore[SEMAPHORES];
	uint8_t *freemem;
	uint16_t cycles;
} kernel;

ISR(SCHEDULE, ISR_NAKED)
{
	struct task *t;
	struct task *rtr;
	uint32_t now;
	uint32_t nexthit;
	int32_t timeleft;

	PUSH_ALL();

	TIMSK &= ~_BV(OCIE1A);		/* turn off output compare 1A */

	if (TIFR & _BV(TOV1)) {
		TIFR |= _BV(TOV1);	/* reset flag */
		++kernel.cycles;
	}

	now = NOW(kernel.cycles, TCNT1);
	nexthit = now + EPOCH;

	/* update idle task */
	kernel.idle->release = now;
	kernel.idle->deadline = nexthit;

	rtr = kernel.idle;

	for (t = kernel.first; t <= kernel.last; t++) {
		/* release tasks from time-wait-queue */
		if (t->state == TIMEQ) {
			if (DISTANCE(now, t->release) < 0)
				t->state = READYQ;
			else if (DISTANCE(t->release, nexthit) > 0)
				nexthit = t->release;
		}

		/* find next task to run */
		if (t->state == READYQ && \
		    DISTANCE(t->deadline, rtr->deadline) > 0)
			rtr = t;
	}

	if (kernel.running != rtr) {
		/* switch task */
		kernel.running->spl = SPL;
		kernel.running->sph = SPH;
		SPL = rtr->spl;
		SPH = rtr->sph;
		kernel.prev = kernel.running;
		kernel.running = rtr;
	}

	now = NOW(kernel.cycles, TCNT1);
	timeleft = DISTANCE(now, nexthit);

	if (timeleft < EPS)
		timeleft = EPS;

	timeleft += TCNT1;

	if (timeleft < 0xFFFF)
		OCR1A = timeleft;
	else if (TCNT1 < 0xFFFF - EPS)
		OCR1A = 0;
	else
		OCR1A = EPS;

	TIMSK |= _BV(OCIE1A);

	POP_ALL();

	reti();
}

void
init(int idlestack)
{
	/* Set up timer 1 */
	TCNT1 = 0;			/* reset counter 1 */
	TCCR1A = 0;			/* normal operation */
	TCCR1B = TIMER_FLAGS;
	TIMSK = _BV(OCIE1A);

	kernel.freemem = (void *)(RAMEND - idlestack);
	kernel.idle = &kernel.task[0];
	kernel.first = &kernel.task[1];
	kernel.last = kernel.idle;
	kernel.running = kernel.idle;
	kernel.prev = kernel.idle;
	kernel.cycles = 0;

	/* Initialize idle task (task 0) */
	kernel.running->release = 0;
	kernel.running->deadline = EPOCH;

	sei();
}

void
task(void (*fun)(void *), uint16_t stack, uint32_t release, uint32_t deadline, void *args)
{
	struct task *t;
	uint8_t *sp;
	int     i;

	cli();

	sp = kernel.freemem;
	kernel.freemem -= stack;

	/*
	*sp-- = 'A';
	*sp-- = 'A';
	 */

	/* initialize stack */
	*sp-- = LO8(fun);		/* store PC(lo) */
	*sp-- = HI8(fun);		/* store PC(hi) */

	for (i = 0; i < 25; i++)
		*sp-- = 0;		/* store r1-r0, SREG, r2-r23 */

	/* Save args in r24-25 (input arguments stored in these registers) */
	*sp-- = LO8(args);
	*sp-- = HI8(args);

	for (i = 0; i < 6; i++)
		*sp-- = 0;		/* store r26-r31 */

	t = ++kernel.last;

	t->release = release;
	t->deadline = deadline;
	t->state = TIMEQ;

	t->spl = LO8(sp);		/* store stack pointer */
	t->sph = HI8(sp);

	SCHEDULE();
}

void
semaphore(uint8_t sema, uint8_t val)
{
	cli();

	kernel.semaphore[sema] = val;

	sei();
}

void
wait(uint8_t sema)
{
	cli();

	if (kernel.semaphore[sema] == 0) {
		kernel.running->state = WAITQOFFSET + sema;
		SCHEDULE();
	} else
		--kernel.semaphore[sema];

	sei();
}

void
signal(uint8_t sema)
{
	struct task *t;
	struct task *rtr;

	cli();

	rtr = kernel.idle;

	for (t = kernel.first; t <= kernel.last; t++) {
		if (t->state == WAITQOFFSET + sema && \
		    DISTANCE(t->deadline, rtr->deadline) > 0)
			rtr = t;
	}

	if (rtr != kernel.idle) {
		rtr->state = READYQ;
		SCHEDULE();
	} else
		++kernel.semaphore[sema];

	sei();
}

void
update(uint32_t release, uint32_t deadline)
{
	cli();

	kernel.running->state = TIMEQ;
	kernel.running->release = release;
	kernel.running->deadline = deadline > release ? deadline : release;

	SCHEDULE();
}

uint32_t
deadline(void)
{
	return kernel.running->deadline;
}

uint32_t
release(void)
{
	return kernel.running->release;
}

uint32_t
now(void)
{
	return NOW(kernel.cycles, TCNT1);
}

void
suspend(void)
{
	cli();

	kernel.running->state = TERMINATED;

	SCHEDULE();
}

uint8_t
running(void)
{
	return kernel.running - kernel.idle;
}

uint8_t
previous(void)
{
	return kernel.prev - kernel.idle;
}
