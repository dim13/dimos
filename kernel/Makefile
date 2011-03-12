# $Id$

PROG		= kernel
SRCS		= kernel.c main.c uart.c heartbeat.c rgb.c hsv.c lcd3.c adc.c
HEADERS		= kernel.h stack.h
MCU_TARGET	= atmega8
F_CPU		= 16000000
PRESCALE	= 8
STACK		= 64
TASKS		= 8
SEMAPHORES	= 8
BAUD		= 9600

# You should not have to change anything below here.

CC		= avr-gcc
OBJCOPY		= avr-objcopy
OBJDUMP		= avr-objdump
SIZE		= avr-size
OBJS		= ${SRCS:.c=.o}
CFLAGS		= -Wall -Os -mmcu=${MCU_TARGET} \
		-DF_CPU=${F_CPU} -DPRESCALE=${PRESCALE} -DBAUD=${BAUD} \
		-DSTACK=${STACK} -DTASKS=${TASKS} -DSEMAPHORES=${SEMAPHORES}
LDFLAGS		= -Wl,-Map,${PROG}.map

.SUFFIXES:	.elf .lst .hex .bin .srec .ehex .ebin .esrec

all: ${PROG}.elf ${PROG}.lst ${PROG}.hex ${PROG}.ehex size

${SRCS}: ${HEADERS} Makefile

${PROG}.elf: ${OBJS}
	${CC} ${LDFLAGS} -o ${.TARGET} ${.ALLSRC}

size: ${PROG}.elf
	${SIZE} -C --mcu=${MCU_TARGET} ${.ALLSRC}

clean:
	rm -f *.o ${PROG}.elf *.bak *.lst *.map *.hex *.bin *srec *.ehex *.ebin *.esrec

.elf.lst:
	${OBJDUMP} -h -S ${.IMPSRC} > ${.TARGET}

# Rules for building the .text rom images

.elf.hex:
	${OBJCOPY} -j .text -j .data -O ihex ${.IMPSRC} ${.TARGET}

.elf.bin:
	${OBJCOPY} -j .text -j .data -O binary ${.IMPSRC} ${.TARGET}

.elf.srec:
	${OBJCOPY} -j .text -j .data -O srec ${.IMPSRC} ${.TARGET}

# Rules for building the .eeprom rom images

.elf.ehex:
	${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O ihex ${.IMPSRC} ${.TARGET}

.elf.ebin:
	${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O binary ${.IMPSRC} ${.TARGET}

.elf.esrec:
	${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O srec ${.IMPSRC} ${.TARGET}
