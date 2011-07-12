# $Id$

MCU=		atmega8
F_CPU=		16000000
PRESCALE=	8
#STACK=		64
STACK=		48
#STACK=		56
TASKS=		8
SEMAPHORES=	8
BAUD=		9600

PROG=	dmx
SRCS=	dmx.c heartbeat.c rgb.c hsv.c \
	clock.c lcd3.c ctrl.c
NOMAN=

.include <bsd.prog.mk>
