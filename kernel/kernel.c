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
#define SPAN(from, to)		((int32_t)((to) - (from)))
#define SLICE			MSEC(1)
#define TIMEOUT			WDTO_500MS
#define swtch			TIMER1_COMPB_vect

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
	uint8_t semaphore;		/* bitmap */
	uint8_t maxid;
	uint8_t reboot;
} kern;

ISR(TIMER1_OVF_vect)
{
	if (!kern.reboot)
		wdt_reset();

	++kern.cycles;
}

ISR(TIMER1_COMPA_vect)
{
	struct task *tp;
	uint32_t now;

	now = NOW(kern.cycles, TCNT1);

	/* release waiting tasks */
	while ((tp = TAILQ_FIRST(&kern.tq)) && SPAN(now, tp->release) <= 0) {
		TAILQ_REMOVE(&kern.tq, tp, t_link);
		TAILQ_INSERT_TAIL(&kern.rq, tp, r_link);
	}

	/* set next wakeup timer */
	if ((tp = TAILQ_FIRST(&kern.tq)))
		OCR1A = TCNT1 + SPAN(now, tp->release);
}

ISR(TIMER1_COMPB_vect, ISR_NAKED)
{
	pusha();

	/* reschedule current task if it've used its time slice */
	if (kern.cur == TAILQ_FIRST(&kern.rq)) {
		TAILQ_REMOVE(&kern.rq, kern.cur, r_link);
		TAILQ_INSERT_TAIL(&kern.rq, kern.cur, r_link);
	}

	/* switch context */
	kern.cur->sp = SP;
	kern.cur = TAILQ_EMPTY(&kern.rq) ? kern.idle : TAILQ_FIRST(&kern.rq);
	SP = kern.cur->sp;

	/* set task slice timeout */
	OCR1B = TCNT1 + SLICE;

	popa();

	reti();
}

void
init(uint8_t sema)
{
	uint8_t i;

	cli();

	/* disable watchdog */
	MCUSR = 0;
	wdt_disable();

	/* turn all on */
	power_all_enable();

	/* set clock prescale to 1 in case CKDIV8 fuse is on */
	clock_prescale_set(clock_div_1);

	/* Set up timer 1 */
	TCNT1 = 0;				/* reset timer */
	TCCR1A = 0;				/* normal operation */
	TCCR1B = TIMER_FLAGS;			/* prescale */
	TIMSK1 = _BV(OCIE1A) | _BV(OCIE1B) | _BV(TOIE1);
	OCR1A = 0;
	OCR1B = 0;

	/* init queues */
	TAILQ_INIT(&kern.rq);
	TAILQ_INIT(&kern.tq);

	kern.wq = calloc(sema, sizeof(struct queue));
	for (i = 0; i < sema; i++)
		TAILQ_INIT(&kern.wq[i]);

	/* init idle task */
	kern.idle = calloc(1, sizeof(struct task));
	kern.idle->id = 0;
	kern.idle->release = 0;
	kern.idle->sp = SP;			/* not really needed */
	kern.cur = kern.idle;

	kern.cycles = 0;
	kern.semaphore = 0;
	kern.maxid = 0;
	kern.reboot = 0;

	wdt_enable(TIMEOUT);

	swtch();
}

void
exec(void (*fun)(void *), void *args, uint8_t stack)
{
	struct task *tp;
	uint8_t *sp;

	cli();

	/* allocate task memory */
	tp = calloc(1, sizeof(struct task));
	tp->stack = calloc(stack, sizeof(uint8_t));
	sp = &tp->stack[stack - 1];

	/* initialize stack */
	*sp-- = LO8(fun);		/* PC(lo) */
	*sp-- = HI8(fun);		/* PC(hi) */

	*sp-- = 0;			/* r1 */
	*sp-- = 0;			/* r0 */
	*sp-- = SREG | _BV(SREG_I);	/* SREG */

	sp -= 22;
	memset(sp, 0, 22);		/* r2-r23 */
	
	*sp-- = LO8(args);		/* r24 */
	*sp-- = HI8(args);		/* r25 */

	sp -= 6;
	memset(sp, 0, 6);		/* r26-r31 */

	tp->id = ++kern.maxid;
	tp->release = 0;
	tp->sp = (uint16_t)sp;		/* SP */
	TAILQ_INSERT_TAIL(&kern.rq, tp, r_link);

	sei();
}

void
wait(uint8_t chan)
{
	cli();

	if (kern.semaphore & _BV(chan)) {
		/* semaphore busy, go into wait queue */
		TAILQ_REMOVE(&kern.rq, kern.cur, r_link);
		TAILQ_INSERT_TAIL(&kern.wq[chan], kern.cur, w_link);
		swtch();
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
	} else {
		/* clear semaphore and continue */
		kern.semaphore &= ~_BV(chan);
	}

	sei();
}

void
sleep(uint32_t sec, uint32_t usec)
{
	struct task *tp;

	cli();

	kern.cur->release = NOW(kern.cycles, TCNT1) + SEC(sec) + USEC(usec);
	TAILQ_REMOVE(&kern.rq, kern.cur, r_link);

	/* find right place */
	TAILQ_FOREACH(tp, &kern.tq, t_link)
		if (SPAN(tp->release, kern.cur->release) < 0)
			break;
	
	if (tp)
		TAILQ_INSERT_BEFORE(tp, kern.cur, t_link);
	else
		TAILQ_INSERT_TAIL(&kern.tq, kern.cur, t_link);

	swtch();
}

void
yield(void)
{
	cli();

	TAILQ_REMOVE(&kern.rq, kern.cur, r_link);
	TAILQ_INSERT_TAIL(&kern.rq, kern.cur, r_link);

	swtch();
}

void
suspend(void)
{
	cli();

	/* TODO: free memory */
	TAILQ_REMOVE(&kern.rq, kern.cur, r_link);

	swtch();
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
	kern.reboot = 1;
}
