#ifndef STACKCHECK_H
#define STACKCHECK_H

static inline void stackcheck(void) {
	__asm
		; Only once do the extern directives, because more than one causes an assembler error.
		ifndef STACKCHECK_ONCE
		extern _stack
		extern _stackcheck_failed
		variable STACKCHECK_ONCE=1
		endif

		; Compare FSR1H to the high byte of the stack segment.
		; If not equal, go over to the failure function.
		movlw HIGH(_stack)
		cpfseq _FSR1H
		goto _stackcheck_failed
	__endasm;
}

#endif

