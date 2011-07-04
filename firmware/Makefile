# $Id$

PROG		= firmware
SRCS		= firmware.c
HEADERS		=
MCU_TARGET	= atmega8
F_CPU		= 16000000
PRESCALE	= 8
BAUD		= 9600
ORG		= 0x1e00

# You should not have to change anything below here.

OBJS		= ${SRCS:.c=.o}
CC		= avr-gcc
CFLAGS		= -Wall -Os -mmcu=${MCU_TARGET} \
		-DF_CPU=${F_CPU} -DPRESCALE=${PRESCALE} -DBAUD=${BAUD}
LDFLAGS		= -Wl,-Map,${PROG}.map,--section-start=.text=${ORG}
OBJCOPY		= avr-objcopy
OBJDUMP		= avr-objdump
SIZE		= avr-size

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
	${OBJDUMP} -h -S ${.ALLSRC} > ${.TARGET}

# Rules for building the .text rom images

.elf.hex:
	${OBJCOPY} -j .text -j .data -O ihex ${.ALLSRC} ${.TARGET}

.elf.bin:
	${OBJCOPY} -j .text -j .data -O binary ${.ALLSRC} ${.TARGET}

.elf.srec:
	${OBJCOPY} -j .text -j .data -O srec ${.ALLSRC} ${.TARGET}

# Rules for building the .eeprom rom images

.elf.ehex:
	${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O ihex ${.ALLSRC} ${.TARGET}

.elf.ebin:
	${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O binary ${.ALLSRC} ${.TARGET}

.elf.esrec:
	${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O srec ${.ALLSRC} ${.TARGET}
