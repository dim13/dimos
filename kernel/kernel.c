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

#define DEBUG	0

enum State { TERMINATED, RUNQ, TIMEQ, WAITQ, SIGNAL };

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
	uint8_t sema;
	TAILQ_ENTRY(task) link;
};

struct kernel {
	TAILQ_HEAD(queue, task) runq[PRIORITIES], timeq, waitq[SEMAPHORES];
	struct task task[1 + TASKS];
	struct task *nextfree;
	struct task *current;
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
	struct task *tp, *tmp;
	int32_t dist;
	uint32_t now;
	uint16_t nexthit;
	uint8_t prio;

	PUSH_ALL();
	now = NOW(kernel.cycles, TCNT1);

	#if DEBUG
	PORTB ^= _BV(PB1);		/* DEBUG */
	#endif

	kernel.current->sp = SP;
	TAILQ_REMOVE(&kernel.runq[kernel.current->prio], kernel.current, link);

	nexthit = 0xffff;

	/* release waiting tasks */
	TAILQ_FOREACH_SAFE(tp, &kernel.timeq, link, tmp) {
		dist = DISTANCE(now, tp->release);
		if (dist <= 0) {
			tp->state = RUNQ;
			TAILQ_REMOVE(&kernel.timeq, tp, link);
			TAILQ_INSERT_TAIL(&kernel.runq[tp->prio], tp, link);
		} else if (dist < nexthit)
			nexthit = dist;
		else
			break;
	}

again:
	switch (kernel.current->state) {
	case RUNQ:
		TAILQ_INSERT_TAIL(&kernel.runq[kernel.current->prio], kernel.current, link);
		break;
	case TIMEQ:
		TAILQ_FOREACH(tp, &kernel.timeq, link)
			if (DISTANCE(kernel.current->release, tp->release) > 0)
				break;

		if (tp)
			TAILQ_INSERT_BEFORE(tp, kernel.current, link);
		else
			TAILQ_INSERT_TAIL(&kernel.timeq, kernel.current, link);
		break;
	case WAITQ:
		if (kernel.semaphore[kernel.current->sema] == 0) {
			/* occupy semaphore */
			kernel.semaphore[kernel.current->sema] = 1;
			kernel.current->state = RUNQ;
			goto again;	/* put current task back on runq */
		} else
			TAILQ_INSERT_TAIL(&kernel.waitq[kernel.current->sema], kernel.current, link);
		break;
	case SIGNAL:
		tp = TAILQ_FIRST(&kernel.waitq[kernel.current->sema]);
		if (tp) {
			tp->state = RUNQ;
			TAILQ_REMOVE(&kernel.waitq[kernel.current->sema], tp, link);
			TAILQ_INSERT_TAIL(&kernel.runq[tp->prio], tp, link);
			/* occupy semaphore */
			kernel.semaphore[kernel.current->sema] = 1;
		} else
			kernel.semaphore[kernel.current->sema] = 0;

		kernel.current->state = RUNQ;
		goto again;		/* put current task back on runq */
	default:
		break;
	}

	for (prio = 0; prio < PRIORITIES - 1; prio++)
		if (!TAILQ_EMPTY(&kernel.runq[prio]))
			break;

	if (TAILQ_EMPTY(&kernel.runq[prio]))
		TAILQ_INSERT_TAIL(&kernel.runq[prio], &kernel.task[0], link);

	kernel.current = TAILQ_FIRST(&kernel.runq[prio]);
	SP = kernel.current->sp;

	OCR1A = (uint16_t)(now + nexthit);

	POP_ALL();
	reti();
}

void
init(uint8_t stack)
{
	uint8_t i;

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

	memset(&kernel, 0, sizeof(kernel));


	for (i = 0; i < PRIORITIES; i++)
		TAILQ_INIT(&kernel.runq[i]);
	TAILQ_INIT(&kernel.timeq);
	for (i = 0; i < SEMAPHORES; i++)
		TAILQ_INIT(&kernel.waitq[i]);

	kernel.nextfree = &kernel.task[1];
	kernel.freemem = (void *)(RAMEND - stack);
	kernel.current = &kernel.task[0];
	kernel.current->state = TERMINATED;
	kernel.current->prio = PRIORITIES - 1;
	kernel.current->release = 0;
	TAILQ_INSERT_TAIL(&kernel.runq[PRIORITIES - 1], &kernel.task[0], link);

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

	t = kernel.nextfree++;
	t->release = NOW(kernel.cycles, TCNT1);
	t->prio = prio;
	t->sema = 0;
	t->sp = (uint16_t)sp;		/* SP */
	t->state = TIMEQ;
	TAILQ_INSERT_TAIL(&kernel.timeq, t, link);

	SCHEDULE();
}

void
wait(uint8_t sema)
{
	cli();

	kernel.current->sema = sema;
	kernel.current->state = WAITQ;

	SCHEDULE();
}

void
signal(uint8_t sema)
{
	cli();

	kernel.current->state = SIGNAL;

	SCHEDULE();
}

void
sleep(uint32_t ticks)
{
	cli();

	kernel.current->release += ticks;
	kernel.current->state = TIMEQ;

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
