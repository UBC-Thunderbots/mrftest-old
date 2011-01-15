#include "serial.h"
#include <pic18fregs.h>

void serial_init(void) {
	/* Clear any cruft left over from a previous execution. */
	RCSTA1 = 0;
	RCSTA2 = 0;
	TXSTA1 = 0;
	TXSTA2 = 0;

	/* Device oscillator frequency is 48,000,000 Hz
	 * Desired baud rate is 250,000 baud
	 * In asynchronous mode with BRGH=1 and BRG16=0, baud = Fosc / (16(n+1))
	 * If n=11, then 48,000,000/(16(11+1)) = 250,000 exactly */
	SPBRG1 = 11;
	SPBRG2 = 11;

	/*         /-------- Ignored
	 *         |/------- 8-bit transmission
	 *         ||/------ Transmitter disabled (until needed)
	 *         |||/----- Asynchronous mode
	 *         ||||/---- Do not send break
	 *         |||||/--- High baud rate mode
	 *         ||||||/-- Transmit shift register empty
	 *         |||||||/- 9th data bit */
	TXSTA1 = 0b00000110;
	TXSTA2 = 0b00000110;

	/*         /-------- Module enabled
	 *         |/------- 8-bit reception
	 *         ||/------ Ignored
	 *         |||/----- Receiver disabled (until needed)
	 *         ||||/---- Ignored
	 *         |||||/--- No framing error
	 *         ||||||/-- No overrun error
	 *         |||||||/- 9th data bit */
	RCSTA1 = 0b10000000;
	RCSTA2 = 0b10000000;

	/* Workaround for errata #3, must ensure that a 2-cycle instruction is not executed immediately after enabling a serial port. */
	Nop();
	Nop();
}

void serial_deinit(void) {
	/* Disable the modules. */
	RCSTA1 = 0;
	RCSTA2 = 0;
}

