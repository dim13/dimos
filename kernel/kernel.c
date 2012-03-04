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
	uint8_t chan;		/* wait channel */
	TAILQ_ENTRY(task) r_link, t_link, w_link;
};

struct kernel {
	TAILQ_HEAD(queue, task) runq, timeq, waitq;
	struct task idle[1 + TASKS];
	struct task *nextfree;
	struct task *current;
	uint16_t cycles;
	uint8_t *freemem;
	uint8_t semaphore;
} kernel;

ISR(TIMER1_OVF_vect)
{
	++kernel.cycles;
}

ISR(TIMER1_COMPA_vect, ISR_NAKED)
{
	struct task *tp, *tmp;
	uint32_t now;
	uint16_t nexthit;
	int32_t dist;

	PUSH_ALL();
	now = NOW(kernel.cycles, TCNT1);

	#if DEBUG
	PORTB ^= _BV(PB1);		/* DEBUG */
	#endif

	if (kernel.current == TAILQ_FIRST(&kernel.runq)) {
		TAILQ_REMOVE(&kernel.runq, kernel.current, r_link);
		if (kernel.current != kernel.idle)
			TAILQ_INSERT_TAIL(&kernel.timeq, kernel.current, t_link);
	}

	/* release waiting tasks */
	TAILQ_FOREACH_SAFE(tp, &kernel.timeq, t_link, tmp) {
		if (DISTANCE(now, tp->release) <= 0) {
			TAILQ_REMOVE(&kernel.timeq, tp, t_link);
			TAILQ_INSERT_TAIL(&kernel.runq, tp, r_link);
		} else
			break;
	}

	/* idle if nothing to run */
	if (TAILQ_EMPTY(&kernel.runq))
		TAILQ_INSERT_TAIL(&kernel.runq, kernel.idle, r_link);

	nexthit = UINT16_MAX;

	if ((tp = TAILQ_FIRST(&kernel.timeq))) {
		dist = DISTANCE(now, tp->release);
		if (dist < nexthit)
			nexthit = dist;
	}

	OCR1A = (uint16_t)(now + nexthit);

	/* switch context */
	kernel.current->sp = SP;
	kernel.current = TAILQ_FIRST(&kernel.runq);
	SP = kernel.current->sp;

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
	kernel.nextfree = kernel.idle + 1;
	kernel.freemem = (void *)(RAMEND - stack);
	kernel.idle->release = 0;
	TAILQ_INSERT_TAIL(&kernel.runq, kernel.idle, r_link);
	kernel.current = TAILQ_FIRST(&kernel.runq);

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
	tp->release = 0;
	tp->chan = 0;
	tp->sp = (uint16_t)sp;		/* SP */
	TAILQ_INSERT_TAIL(&kernel.runq, tp, r_link);

	SCHEDULE();
}

void
wait(uint8_t chan)
{
	cli();

	TAILQ_REMOVE(&kernel.runq, kernel.current, r_link);
	kernel.current->chan = chan;

	if (kernel.semaphore & _BV(kernel.current->chan)) {
		/* semaphore busy, go into wait queue */
		TAILQ_INSERT_TAIL(&kernel.waitq, kernel.current, w_link);
	} else {
		/* occupy semaphore */
		kernel.semaphore |= _BV(kernel.current->chan);
		TAILQ_INSERT_TAIL(&kernel.runq, kernel.current, r_link);
	}

	SCHEDULE();
}

void
signal(void)
{
	struct task *tp, *tmp;

	cli();

	TAILQ_REMOVE(&kernel.runq, kernel.current, r_link);

	/* release waiting tasks from wait queue */
	TAILQ_FOREACH_SAFE(tp, &kernel.waitq, w_link, tmp) {
		if (tp->chan == kernel.current->chan) {
			TAILQ_REMOVE(&kernel.waitq, tp, w_link);
			TAILQ_INSERT_TAIL(&kernel.runq, tp, r_link);
		}
	}

	/* clear semaphore, and go back to run queue */
	kernel.semaphore &= ~_BV(kernel.current->chan);
	TAILQ_INSERT_TAIL(&kernel.runq, kernel.current, r_link);

	SCHEDULE();
}

void
sleep(uint32_t sec, uint32_t usec)
{
	struct task *tp;

	cli();

	TAILQ_REMOVE(&kernel.runq, kernel.current, r_link);
	kernel.current->release += SEC(sec) + USEC(usec);

	/* find right position on time queue */
	TAILQ_FOREACH(tp, &kernel.timeq, t_link)
		if (DISTANCE(kernel.current->release, tp->release) > 0)
			break;

	if (tp)
		TAILQ_INSERT_BEFORE(tp, kernel.current, t_link);
	else
		TAILQ_INSERT_TAIL(&kernel.timeq, kernel.current, t_link);

	SCHEDULE();
}

void
yield(void)
{
	cli();

	TAILQ_REMOVE(&kernel.runq, kernel.current, r_link);
	TAILQ_INSERT_TAIL(&kernel.timeq, kernel.current, t_link);

	SCHEDULE();
}

void
suspend(void)
{
	cli();

	TAILQ_REMOVE(&kernel.runq, kernel.current, r_link);

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
	return kernel.current - kernel.idle;
}
