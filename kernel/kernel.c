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

#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "kernel.h"
#include "stack.h"

enum State { TERMINATED, RUNQ, TIMEQ, WAITQOFFSET };

#define LO8(x)			((uint8_t)((uint16_t)(x)))
#define HI8(x)			((uint8_t)((uint16_t)(x) >> 8))
#define SCHEDULE		TIMER1_COMPA_vect
#define DISTANCE(from, to)	((int32_t)((to) - (from)))
#define EPOCH			(INT32_MAX >> 1)
#define NOW(hi, lo)		(((uint32_t)(hi) << 0x10) | (lo))

struct task {
	uint32_t release;
	uint32_t deadline;
	uint16_t sp;		/* stack pointer */
	uint8_t state;
};

struct kernel {
	struct task *running;
	struct task *last;
	struct task task[TASKS + 1];
	uint16_t cycles;
	uint8_t *freemem;
	uint8_t semaphore[SEMAPHORES];
} kernel;

ISR(TIMER1_OVF_vect)
{
	++kernel.cycles;
}

ISR(TIMER1_COMPA_vect, ISR_NAKED)
{
	struct task *t;
	struct task *rtr;
	uint32_t now;
	uint32_t nexthit;

	PUSH_ALL();

	now = NOW(kernel.cycles, TCNT1);
	nexthit = EPOCH + now;

	/* update idle task */
	kernel.task->deadline = nexthit;

	rtr = kernel.task;

	for (t = &kernel.task[1]; t <= kernel.last; t++) {
		/* release tasks from time-wait-queue */
		if (t->state == TIMEQ) {
			if (DISTANCE(t->release, now) > 0)
				t->state = RUNQ;
			else if (DISTANCE(t->release, nexthit) > 0)
				nexthit = t->release;
		}

		/* find next task to run */
		if (t->state == RUNQ) {
			if (DISTANCE(t->deadline, rtr->deadline) > 0)
				rtr = t;
		}
	}

	/* switch task */
	kernel.running->sp = SP;
	SP = rtr->sp;
	kernel.running = rtr;

	OCR1A = (uint16_t)nexthit;
	
	POP_ALL();
	reti();
}

void
init(uint8_t stack)
{
	cli();

	/* Set up timer 1 */
	TCNT1 = 0;				/* reset timer */
	TCCR1A = 0;				/* normal operation */
	TCCR1B = TIMER_FLAGS;			/* prescale */
	TIMSK = (_BV(OCIE1A) | _BV(TOIE1));	/* enable interrupts */

	kernel.cycles = 0;
	kernel.freemem = (void *)(RAMEND - stack);
	kernel.last = kernel.task;
	kernel.running = kernel.task;

	/* Initialize idle task (task 0) */
	kernel.running->deadline = EPOCH;
	kernel.running->state = RUNQ;

	sei();
}

void
exec(void (*fun)(void *), uint8_t stack, void *args)
{
	struct task *t;
	uint8_t *sp;

	cli();

	sp = kernel.freemem;
	kernel.freemem -= stack;

	/* initialize stack */
	*sp-- = LO8(fun);		/* PC(lo) */
	*sp-- = HI8(fun);		/* PC(hi) */

	sp -= 25;
	memset(sp, 0, 25);		/* r1, r0, SREG, r2-r23 */
	
	*sp-- = LO8(args);		/* r24 */
	*sp-- = HI8(args);		/* r25 */

	sp -= 6;
	memset(sp, 0, 6);		/* r26-r31 */

	t = ++kernel.last;

	t->release = 0;
	t->deadline = EPOCH;
	t->state = TIMEQ;

	t->sp = (uint16_t)sp;		/* SP */

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
	} else {
		--kernel.semaphore[sema];
		sei();
	}
}

void
signal(uint8_t sema)
{
	struct task *t, *rtr;

	cli();

	rtr = kernel.task;

	for (t = &kernel.task[1]; t <= kernel.last; t++) {
		if (t->state == WAITQOFFSET + sema)
			if (DISTANCE(t->deadline, rtr->deadline) > 0)
				rtr = t;
	}

	if (rtr != kernel.task) {
		rtr->state = RUNQ;
		SCHEDULE();
	} else {
		++kernel.semaphore[sema];
		sei();
	}
}

void
set(uint32_t release, uint32_t deadline)
{
	cli();

	kernel.running->state = TIMEQ;
	kernel.running->release = release;
	kernel.running->deadline = deadline;

	SCHEDULE();
}

void
update(uint32_t release, uint32_t deadline)
{
	cli();

	kernel.running->state = TIMEQ;
	kernel.running->release += release;
	kernel.running->deadline = kernel.running->release + deadline;

	SCHEDULE();
}

uint32_t
deadline(void)
{
	uint32_t ret;

	cli();
	ret = kernel.running->deadline;
	sei();

	return ret;
}

uint32_t
release(void)
{
	uint32_t ret;

	cli();
	ret = kernel.running->release;
	sei();

	return ret;
}

uint32_t
now(void)
{
	uint32_t ret;

	cli();
	ret = NOW(kernel.cycles, TCNT1);
	sei();

	return ret;
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
	uint8_t ret;

	cli();
	ret = kernel.running - kernel.task;
	sei();

	return ret;
}
