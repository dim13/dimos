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
#include <avr/power.h>
#include <avr/wdt.h>
#include "kernel.h"
#include "stack.h"
#include "queue.h"

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
	TAILQ_ENTRY(task) r_link, t_link, w_link;
};

struct kernel {
	TAILQ_HEAD(queue, task) runq, timeq, waitq[SEMAPHORES];
	struct task idle[1 + TASKS];
	struct task *last;
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

	pusha();
	now = NOW(kernel.cycles, TCNT1);
	nexthit = UINT16_MAX;

	TAILQ_FOREACH_SAFE(tp, &kernel.timeq, t_link, tmp) {
		dist = DISTANCE(now, tp->release);
		if (dist <= 0) {
			TAILQ_REMOVE(&kernel.timeq, tp, t_link);
			TAILQ_INSERT_TAIL(&kernel.runq, tp, r_link);
		} else if (dist < nexthit)
			nexthit = dist;
	}

	if (kernel.current == TAILQ_FIRST(&kernel.runq)) {
		TAILQ_REMOVE(&kernel.runq, kernel.current, r_link);
		if (kernel.current != kernel.idle)
			TAILQ_INSERT_TAIL(&kernel.runq, kernel.current, r_link);
	}

	if (TAILQ_EMPTY(&kernel.runq))
		TAILQ_INSERT_TAIL(&kernel.runq, kernel.idle, r_link);
	
	OCR1A = now + nexthit;

	kernel.current->sp = SP;
	kernel.current = TAILQ_FIRST(&kernel.runq);
	SP = kernel.current->sp;

	popa();
	reti();
}

void
init(uint8_t stack)
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

	TAILQ_INIT(&kernel.runq);
	TAILQ_INIT(&kernel.timeq);
	for (i = 0; i < SEMAPHORES; i++)
		TAILQ_INIT(&kernel.waitq[i]);

	kernel.idle->release = 0;
	kernel.idle->sp = SP;			/* XXX not needed at all */
	TAILQ_INSERT_TAIL(&kernel.runq, kernel.idle, r_link);
	kernel.current = TAILQ_FIRST(&kernel.runq);
	kernel.last = kernel.idle;

	kernel.cycles = 0;
	kernel.freemem = (uint8_t *)(RAMEND - stack);
	kernel.semaphore = 0;

	sei();
}

void
exec(void (*fun)(void *), void *args, uint8_t stack)
{
	struct task *tp;
	uint8_t *sp;

	cli();

	sp = kernel.freemem;
	kernel.freemem -= stack + 2;	/* +PC */

	/* initialize stack */
	*sp-- = LO8(fun);		/* PC(lo) */
	*sp-- = HI8(fun);		/* PC(hi) */

	sp -= 25;
	memset(sp, 0, 25);		/* r1, r0, SREG, r2-r23 */
	
	*sp-- = LO8(args);		/* r24 */
	*sp-- = HI8(args);		/* r25 */

	sp -= 6;
	memset(sp, 0, 6);		/* r26-r31 */

	tp = ++kernel.last;
	tp->release = 0;
	tp->sp = (uint16_t)sp;		/* SP */
	TAILQ_INSERT_TAIL(&kernel.runq, tp, r_link);

	SCHEDULE();
}

void
wait(uint8_t chan)
{
	cli();

	if (kernel.semaphore & _BV(chan)) {
		/* semaphore busy, go into wait queue */
		TAILQ_REMOVE(&kernel.runq, kernel.current, r_link);
		TAILQ_INSERT_TAIL(&kernel.waitq[chan], kernel.current, w_link);
		SCHEDULE();
	} else {
		/* occupy semaphore and continue */
		kernel.semaphore |= _BV(chan);
		sei();
	}
}

void
signal(uint8_t chan)
{
	struct task *tp;

	cli();

	if ((tp = TAILQ_FIRST(&kernel.waitq[chan]))) {
		/* release first waiting task from wait queue */
		TAILQ_REMOVE(&kernel.waitq[chan], tp, w_link);
		TAILQ_INSERT_TAIL(&kernel.runq, tp, r_link);
		SCHEDULE();
	} else {
		/* clear semaphore and continue */
		kernel.semaphore &= ~_BV(chan);
		sei();
	}
}

void
sleep(uint32_t sec, uint32_t usec)
{
	cli();

	kernel.current->release += SEC(sec) + USEC(usec);
	TAILQ_REMOVE(&kernel.runq, kernel.current, r_link);
	TAILQ_INSERT_TAIL(&kernel.timeq, kernel.current, t_link);
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

void
reboot(void)
{
	wdt_enable(WDTO_15MS);
}
