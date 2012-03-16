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
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include "kernel.h"
#include "stack.h"
#include "queue.h"

#define LO8(x)			((uint8_t)((uint16_t)(x)))
#define HI8(x)			((uint8_t)((uint16_t)(x) >> 8))
#define NOW(hi, lo)		(((uint32_t)(hi) << 0x10) | (lo))
#define DISTANCE(from, to)	((int32_t)((to) - (from)))
#define SCHEDULE		TIMER1_COMPA_vect

struct task {
	uint32_t release;		/* release time */
	uint16_t sp;			/* stack pointer */
	uint8_t *stack;			/* stack area */
	uint8_t id;			/* task id */
	TAILQ_ENTRY(task) r_link;
	TAILQ_ENTRY(task) t_link;
	TAILQ_ENTRY(task) w_link;
};

TAILQ_HEAD(queue, task);

struct kern {
	struct queue rq;		/* run queue */
	struct queue tq;		/* time queue */
	struct queue *wq;		/* wait queues */
	struct task *idle;
	struct task *cur;		/* current task */
	uint16_t cycles;		/* clock high byte */
	uint16_t semaphore;		/* bitmap */
} kern;

ISR(TIMER1_OVF_vect)
{
	++kern.cycles;
}

ISR(TIMER1_COMPA_vect, ISR_NAKED)
{
	struct task *tp, *tmp;
	uint32_t now;
	uint16_t nexthit;
	int32_t dist;

	pusha();
	now = NOW(kern.cycles, TCNT1);
	nexthit = UINT16_MAX;

	TAILQ_FOREACH_SAFE(tp, &kern.tq, t_link, tmp) {
		dist = DISTANCE(now, tp->release);
		if (dist <= 0) {
			TAILQ_REMOVE(&kern.tq, tp, t_link);
			TAILQ_INSERT_TAIL(&kern.rq, tp, r_link);
		} else if (dist < nexthit)
			nexthit = dist;
	}

	if (kern.cur == TAILQ_FIRST(&kern.rq)) {
		TAILQ_REMOVE(&kern.rq, kern.cur, r_link);
		if (kern.cur != kern.idle) {
			TAILQ_INSERT_TAIL(&kern.rq, kern.cur, r_link);
		}
	}

	if (TAILQ_EMPTY(&kern.rq))
		TAILQ_INSERT_TAIL(&kern.rq, kern.idle, r_link);
	
	OCR1A = now + nexthit;

	kern.cur->sp = SP;
	kern.cur = TAILQ_FIRST(&kern.rq);
	SP = kern.cur->sp;

	popa();
	reti();
}

void
init(uint8_t prio, uint8_t sema)
{
	uint8_t i;

	cli();

	MCUSR = 0;
	wdt_disable();

	clock_prescale_set(clock_div_1);

	/* Set up timer 1 */
	TCNT1 = 0;				/* reset timer */
	TCCR1A = 0;				/* normal operation */
	TCCR1B = TIMER_FLAGS;			/* prescale */
	TIMSK1 = (_BV(OCIE1A) | _BV(TOIE1));	/* enable interrupts */
	OCR1A = 0;				/* default overflow */

	TAILQ_INIT(&kern.rq);
	TAILQ_INIT(&kern.tq);
	kern.wq = calloc(sema, sizeof(struct queue));
	for (i = 0; i < sema; i++)
		TAILQ_INIT(&kern.wq[i]);

	kern.idle = calloc(1, sizeof(struct task));
	kern.idle->id = 0;
	kern.idle->release = 0;
	kern.idle->sp = SP;			/* not really needed */
	TAILQ_INSERT_TAIL(&kern.rq, kern.idle, r_link);
	kern.cur = TAILQ_FIRST(&kern.rq);

	kern.cycles = 0;
	kern.semaphore = 0;

	sei();
}

void
exec(void (*fun)(void *), void *args, uint8_t stack)
{
	struct task *tp;
	uint8_t *sp;
	static uint8_t id = 0;

	cli();

	tp = calloc(1, sizeof(struct task));
	tp->stack = calloc(stack, sizeof(uint8_t));
	sp = tp->stack + stack - 1;

	/* initialize stack */
	*sp-- = LO8(fun);		/* PC(lo) */
	*sp-- = HI8(fun);		/* PC(hi) */

	sp -= 25;
	memset(sp, 0, 25);		/* r1, r0, SREG, r2-r23 */
	
	*sp-- = LO8(args);		/* r24 */
	*sp-- = HI8(args);		/* r25 */

	sp -= 6;
	memset(sp, 0, 6);		/* r26-r31 */

	tp->id = ++id;
	tp->release = 0;
	tp->sp = (uint16_t)sp;		/* SP */
	TAILQ_INSERT_TAIL(&kern.rq, tp, r_link);

	SCHEDULE();
}

void
wait(uint8_t chan)
{
	cli();

	if (kern.semaphore & _BV(chan)) {
		/* semaphore busy, go into wait queue */
		TAILQ_REMOVE(&kern.rq, kern.cur, r_link);
		TAILQ_INSERT_TAIL(&kern.wq[chan], kern.cur, w_link);
		SCHEDULE();
	} else {
		/* occupy semaphore and continue */
		kern.semaphore |= _BV(chan);
		sei();
	}
}

void
signal(uint8_t chan)
{
	struct task *tp;

	cli();

	if ((tp = TAILQ_FIRST(&kern.wq[chan]))) {
		/* release first waiting task from wait queue */
		TAILQ_REMOVE(&kern.wq[chan], tp, w_link);
		TAILQ_INSERT_TAIL(&kern.rq, tp, r_link);
		SCHEDULE();
	} else {
		/* clear semaphore and continue */
		kern.semaphore &= ~_BV(chan);
		sei();
	}
}

void
sleep(uint32_t sec, uint32_t usec)
{
	cli();

	kern.cur->release += SEC(sec) + USEC(usec);
	TAILQ_REMOVE(&kern.rq, kern.cur, r_link);
	TAILQ_INSERT_TAIL(&kern.tq, kern.cur, t_link);
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

	/* TODO: clean up */
	TAILQ_REMOVE(&kern.rq, kern.cur, r_link);
	SCHEDULE();
}

uint32_t
now(void)
{
	return NOW(kern.cycles, TCNT1);
}

uint8_t
running(void)
{
	return kern.cur->id;
}

void
reboot(void)
{
	wdt_enable(WDTO_15MS);
}
