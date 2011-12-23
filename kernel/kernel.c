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
#include "queue.h"

#define DEBUG	1
#define NPRIO	2

enum State { TERMINATED, RTR, RUNQ, SLEEPING, WAITING };

#define LO8(x)			((uint8_t)((uint16_t)(x)))
#define HI8(x)			((uint8_t)((uint16_t)(x) >> 8))
#define SCHEDULE		TIMER1_COMPA_vect
#define DISTANCE(from, to)	((int32_t)((to) - (from)))
#define EPOCH			(INT32_MAX >> 1)
#define NOW(hi, lo)		(((uint32_t)(hi) << 0x10) | (lo))

struct task {
	uint32_t release;
	uint16_t sp;		/* stack pointer */
	uint8_t state;
	uint8_t prio;
	SIMPLEQ_ENTRY(task) link;
};

struct kernel {
	SIMPLEQ_HEAD(, task) runq[NPRIO];
	struct task *last;	/* last allocated task */
	struct task *current;
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
	struct task *tp;
	int32_t dist;
	uint32_t now;
	uint16_t nexthit;
	uint8_t prio;

	/* save context */
	PUSH_ALL();

	now = NOW(kernel.cycles, TCNT1);

	#if DEBUG
	PORTB ^= _BV(PB1);		/* DEBUG */
	#endif

	/* save stack pointer and drop task from run-queue */
	kernel.current->sp = SP;
	if (kernel.current->state == RUNQ)
		kernel.current->state = RTR;
	SIMPLEQ_REMOVE_HEAD(&kernel.runq[kernel.current->prio], link);

	nexthit = 0xffff;
	prio = 0;

	/* walk through tasks and assemble run-queue */
	for (tp = &kernel.task[1]; tp <= kernel.last; tp++) {
		if (tp->state == SLEEPING) {
			dist = DISTANCE(now, tp->release);
			if (dist <= 0)
				tp->state = RTR;
			else if (dist < nexthit)
				nexthit = dist;
		}
		if (tp->state == RTR) {
			/* find highest priority */
			if (tp->prio > prio)
				prio = tp->prio;
			/* put task on queue */
			SIMPLEQ_INSERT_TAIL(&kernel.runq[tp->prio], tp, link);
			tp->state = RUNQ;
		}
	}

	/* idle if all queues empty */
	if (prio == 0 && SIMPLEQ_EMPTY(&kernel.runq[prio])) {
		SIMPLEQ_INSERT_TAIL(&kernel.runq[prio], kernel.task, link);
		kernel.task->state = RUNQ;
	}

	/* pick highest priority and restore stack pointer */
	kernel.current = SIMPLEQ_FIRST(&kernel.runq[prio]);
	SP = kernel.current->sp;

	OCR1A = (uint16_t)(now + nexthit);

	/* restore context */
	POP_ALL();
	reti();
}

void
init(uint8_t stack)
{
	uint8_t	prio;

	cli();

	/* Set up timer 1 */
	TCNT1 = 0;				/* reset timer */
	TCCR1A = 0;				/* normal operation */
	TCCR1B = TIMER_FLAGS;			/* prescale */
	TIMSK = (_BV(OCIE1A) | _BV(TOIE1));	/* enable interrupts */
	OCR1A = 0;				/* default overflow */

	#if DEBUG
	DDRB |= _BV(PB1);		/* DEBUG */
	#endif

	for (prio = 0; prio < NPRIO; prio++)
		SIMPLEQ_INIT(&kernel.runq[prio]);

	kernel.cycles = 0;
	kernel.freemem = (void *)(RAMEND - stack);
	kernel.task->release = 0;
	kernel.task->prio = 0;
	kernel.task->state = RUNQ;

	SIMPLEQ_INSERT_TAIL(&kernel.runq[0], kernel.task, link);

	kernel.last = kernel.task;
	kernel.current = kernel.task;

	sei();
}

void
exec(void (*fun)(void *), void *args, uint8_t stack, uint8_t prio)
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

	if (prio >= NPRIO)
		prio = NPRIO - 1;
	t->prio = prio;

	t->state = RTR;

	t->sp = (uint16_t)sp;		/* SP */

	SCHEDULE();
}

#if 0
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
		kernel.running->state = WAITQ + sema;
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
		if (t->state == WAITQ + sema)
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
#endif

void
sleep(uint32_t ticks)
{
	cli();

	kernel.current->release += ticks;
	kernel.current->state = SLEEPING;

	SCHEDULE();
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

	kernel.current->state = TERMINATED;
	SCHEDULE();
}

uint8_t
running(void)
{
	return kernel.current - kernel.task;
}
