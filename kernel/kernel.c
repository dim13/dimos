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
	uint8_t chan;		/* wait channel */
	TAILQ_ENTRY(task) link;
};

struct kernel {
	TAILQ_HEAD(queue, task) runq, timeq, waitq;
	struct task task[1 + TASKS];
	struct task *nextfree;
	uint16_t cycles;
	uint8_t *freemem;
	uint8_t semaphore;
} kernel;

void
setrq(struct task *tp)
{
	tp->state = RUNQ;
	TAILQ_INSERT_TAIL(&kernel.runq, tp, link);
}

ISR(TIMER1_OVF_vect)
{
	++kernel.cycles;
}

ISR(TIMER1_COMPA_vect, ISR_NAKED)
{
	struct task *tp, *tmp, *cur;
	uint32_t now;
	uint16_t nexthit;
	int32_t dist;

	PUSH_ALL();
	now = NOW(kernel.cycles, TCNT1);

	#if DEBUG
	PORTB ^= _BV(PB1);		/* DEBUG */
	#endif

	cur = TAILQ_FIRST(&kernel.runq);
	TAILQ_REMOVE(&kernel.runq, cur, link);

	/* release waiting tasks */
	TAILQ_FOREACH_SAFE(tp, &kernel.timeq, link, tmp) {
		if (DISTANCE(now, tp->release) <= 0) {
			TAILQ_REMOVE(&kernel.timeq, tp, link);
			setrq(tp);
		} else
			break;
	}

	switch (cur->state) {
	case RUNQ:
		/* readd current task at the end of run queue */
		if (cur != &kernel.task[0])
			setrq(cur);

		break;
	case TIMEQ:
		/* find right position on time queue */
		TAILQ_FOREACH(tp, &kernel.timeq, link)
			if (DISTANCE(cur->release, tp->release) > 0)
				break;

		if (tp)
			TAILQ_INSERT_BEFORE(tp, cur, link);
		else
			TAILQ_INSERT_TAIL(&kernel.timeq, cur, link);

		break;
	case WAITQ:
		if (kernel.semaphore & _BV(cur->chan)) {
			/* semaphore busy, go into wait queue */
			TAILQ_INSERT_TAIL(&kernel.waitq, cur, link);
		} else {
			/* occupy semaphore */
			kernel.semaphore |= _BV(cur->chan);
			/* go into the end of run queue again */
			setrq(cur);
		}

		break;
	case SIGNAL:
		/* release waiting tasks from wait queue */
		TAILQ_FOREACH_SAFE(tp, &kernel.waitq, link, tmp) {
			if (tp->chan == cur->chan) {
				TAILQ_REMOVE(&kernel.waitq, tp, link);
				setrq(tp);
			}
		}

		/* clear semaphore, and go back to run queue */
		kernel.semaphore &= ~_BV(cur->chan);
		setrq(cur);

		break;
	default:
		break;
	}

	/* idle if nothing to run */
	if (TAILQ_EMPTY(&kernel.runq))
		setrq(&kernel.task[0]);

	nexthit = UINT16_MAX;

	if ((tp = TAILQ_FIRST(&kernel.timeq))) {
		dist = DISTANCE(now, tp->release);
		if (dist < nexthit)
			nexthit = dist;
	}

	OCR1A = (uint16_t)(now + nexthit);

	/* switch context */
	cur->sp = SP;
	SP = TAILQ_FIRST(&kernel.runq)->sp;

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
	OCR1A = 0;				/* default overflow */

	#if DEBUG
	DDRB |= _BV(PB1);		/* DEBUG */
	#endif

	memset(&kernel, 0, sizeof(kernel));

	TAILQ_INIT(&kernel.runq);
	TAILQ_INIT(&kernel.timeq);
	TAILQ_INIT(&kernel.waitq);

	kernel.cycles = 0;
	kernel.nextfree = &kernel.task[1];
	kernel.freemem = (void *)(RAMEND - stack);
	kernel.task->state = RUNQ;
	kernel.task->release = 0;
	TAILQ_INSERT_TAIL(&kernel.runq, &kernel.task[0], link);

	sei();
}

void
exec(void (*fun)(void *), void *args, uint8_t stack)
{
	struct task *tp;
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

	tp = kernel.nextfree++;
	tp->release = NOW(kernel.cycles, TCNT1);
	tp->chan = 0;
	tp->sp = (uint16_t)sp;		/* SP */
	tp->state = TIMEQ;
	TAILQ_INSERT_TAIL(&kernel.timeq, tp, link);

	SCHEDULE();
}

void
wait(uint8_t chan)
{
	cli();

	TAILQ_FIRST(&kernel.runq)->chan = chan;
	TAILQ_FIRST(&kernel.runq)->state = WAITQ;

	SCHEDULE();
}

void
signal(void)
{
	cli();

	TAILQ_FIRST(&kernel.runq)->state = SIGNAL;

	SCHEDULE();
}

void
sleep(uint32_t sec, uint32_t usec)
{
	cli();

	TAILQ_FIRST(&kernel.runq)->release += SEC(sec) + USEC(usec);
	TAILQ_FIRST(&kernel.runq)->state = TIMEQ;

	SCHEDULE();
}

void
yield(void)
{
	cli();

	SCHEDULE();
}

void
suspend(void)
{
	cli();

	TAILQ_FIRST(&kernel.runq)->state = TERMINATED;

	SCHEDULE();
}

uint32_t
now(void)
{
	return NOW(kernel.cycles, TCNT1);
}

uint8_t
running(void)
{
	return TAILQ_FIRST(&kernel.runq) - kernel.task;
}
