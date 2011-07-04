# $Id$

MCU=		atmega8
F_CPU=		16000000
PRESCALE=	8
STACK=		64
TASKS=		8
SEMAPHORES=	8
BAUD=		9600

PROG=	kernel
SRCS=	kernel.c main.c uart.c heartbeat.c rgb.c hsv.c lcd3.c \
	adc.c ppm.c
NOMAN=

.include <bsd.prog.mk>
