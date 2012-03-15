# $Id$

MCU=		atmega328p
F_CPU=		20000000
PRESCALE=	8	# res 0.5usec	cycle 32.7msec
STACK=		48	# min stack size
NTASK=		11	# +1 idle = 12
NSEMA=		2
NPRIO=		2
BAUD=		9600

PROG=	dmx
SRCS=	dmx.c heartbeat.c rgb.c hsv.c clock.c lcd3.c uart.c adc.c ctrl.c
#	ppm.c
NOMAN=

.include <bsd.prog.mk>
