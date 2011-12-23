# $Id$

MCU=		atmega8
F_CPU=		16000000
#F_CPU=		14745600
#PRESCALE=	1	# res 62.5nsec	cycle 4msec
PRESCALE=	8	# res 0.5usec	cycle 32.7msec
#PRESCALE=	64	# res 4usec	cycle 262msec
#PRESCALE=	256	# res 16msec	cycle 1 sec
#PRESCALE=	1024	# res 64msec	cycle 4 sec
STACK=		48
TASKS=		9	# +1 idle
SEMAPHORES=	4
BAUD=		9600

PROG=	dmx
SRCS=	dmx.c heartbeat.c rgb.c hsv.c clock.c lcd3.c uart.c adc.c ppm.c
NOMAN=

.include <bsd.prog.mk>
