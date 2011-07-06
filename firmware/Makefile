# $Id$

PROG=		firmware

MCU_TARGET=	atmega8
F_CPU=		16000000
PRESCALE=	8
BAUD=		9600
ORG=		0x1e00

# You should not have to change anything below here.

CC=		avr-gcc
OBJCOPY=	avr-objcopy
OBJDUMP=	avr-objdump
SIZE=		avr-size

CFLAGS=		-Wall -Os -mmcu=${MCU_TARGET} \
		-DF_CPU=${F_CPU} -DPRESCALE=${PRESCALE} -DBAUD=${BAUD}
LDFLAGS=	-Wl,-Map,${PROG}.map,--section-start=.text=${ORG}

.SUFFIXES:	.lst .hex .bin .srec .ehex .ebin .esrec

all: ${PROG} ${PROG}.lst ${PROG}.hex ${PROG}.ehex size

size: ${PROG}
	${SIZE} -C --mcu=${MCU} ${.ALLSRC}

clean:
	rm -f *.o ${PROG} *.bak *.lst *.map *.hex *.bin *srec *.ehex *.ebin *.esrec

${PROG}.lst: ${PROG}
	${OBJDUMP} -h -S ${.ALLSRC} > ${.TARGET}

# Rules for building the .text rom images

${PROG}.hex: ${PROG}
	${OBJCOPY} -j .text -j .data -O ihex ${.ALLSRC} ${.TARGET}

${PROG}.bin: ${PROG}
	${OBJCOPY} -j .text -j .data -O binary ${.ALLSRC} ${.TARGET}

${PROG}.srec: ${PROG}
	${OBJCOPY} -j .text -j .data -O srec ${.ALLSRC} ${.TARGET}

# Rules for building the .eeprom rom images

${PROG}.ehex: ${PROG}
	${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O ihex ${.ALLSRC} ${.TARGET}

${PROG}.ebin: ${PROG}
	${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O binary ${.ALLSRC} ${.TARGET}

${PROG}.esrec: ${PROG}
	${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O srec ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>
