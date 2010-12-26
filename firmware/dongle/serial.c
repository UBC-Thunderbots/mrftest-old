#include "serial.h"
#include <pic18fregs.h>

void serial_init(void) {
	/* Enable the modules. */
	RCSTA1bits.SPEN = 1;
	RCSTA2bits.SPEN = 1;

	/* Workaround for errata #3, must ensure that a 2-cycle instruction is not executed immediately after enabling a serial port. */
	Nop();
	Nop();

	/* Device oscillator frequency is 48,000,000 Hz
	 * Desired baud rate is 250,000 baud
	 * In asynchronous mode with BRGH=1 and BRG16=0, baud = Fosc / (16(n+1))
	 * If n=11, then 48,000,000/(16(11+1)) = 250,000 exactly */
	TXSTA1bits.BRGH = 1;
	TXSTA2bits.BRGH = 1;
	SPBRG1 = 11;
	SPBRG2 = 11;
}

void serial_deinit(void) {
	/* Disable the modules. */
	RCSTA1bits.SPEN = 0;
	RCSTA2bits.SPEN = 0;
}

