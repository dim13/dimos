# $Id$

MCU=		atmega8
F_CPU=		16000000
PRESCALE=	8
STACK=		64
TASKS=		8
SEMAPHORES=	8
BAUD=		9600
USE_2X=		1

PROG=	dmx
SRCS=	dmx.c uart.c heartbeat.c rgb.c hsv.c cmd.c
NOMAN=

.include <bsd.prog.mk>
