	; asmsyntax=pic

#include "externdef.inc"

	; ========================
	; DEBUG PRINTING FUNCTIONS
	; ========================
	;
	; These functions send diagnostic data (typically short strings) to a
	; serial port. The serial port is on RB7, which corresponds to the ICSP
	; pin on the PICkit2 that acts as a serial receive pin when the PICkit2's
	; UART tool is activated. Thus strings sent out by these functions can be
	; seen in an attached PICkit2's UART tool.
	;
	; The data is sent out at 9600 bps and uses timer 3 for timing.



	; function dbgprint_init
	; ======================
	;
	; Initializes the debug printing serial port.
	EXTERNDEF dbgprint_init



	; function dbgprint_rom
	; =====================
	;
	; Prints a string stored in code ROM to the debug serial port.
	;
	; On entry:
	;  TBLPTR -> the ASCIZ string to print
	;
	; On exit:
	;  BSR destroyed
	;  FSR0 destroyed
	;  TABLAT destroyed
	;
	; Timing:
	;  Returns immediately. If the transmit queue is full, message is dropped.
	EXTERNDEF dbgprint_rom



	; macro DBGPRINT_ROM
	; ==================
	;
	; Prints a string stored in code ROM to the debug serial port.
	; This is a convenience macro to load an address onto TBLPTR and
	; call the dbgprint_rom function.
DBGPRINT_ROM macro message
	movlw HIGH(message)
	movwf TBLPTRH
	movlw LOW(message)
	movwf TBLPTRL
	call dbgprint_rom
	endm



	; function dbgprint_hex
	; =====================
	;
	; Prints, in hexadecimal, the value of WREG.
	EXTERNDEF dbgprint_hex



	; macro DBGPRINT_HEX
	; ==================
	;
	; Wrapper around dbgprint_hex.
DBGPRINT_HEX macro
	call dbgprint_hex
	endm



	; function dbgprint_nl
	; ====================
	;
	; Prints a newline.
	EXTERNDEF dbgprint_nl



	; macro DBGPRINT_NL
	; =================
	;
	; Wrapper around dbgprint_nl.
DBGPRINT_NL macro
	call dbgprint_nl
	endm



	; function dbgprint_flush
	; =======================
	;
	; Waits until all pending data has been printed.
	EXTERNDEF dbgprint_flush



	; function timer3_int
	; ===================
	;
	; Must be invoked on a TIMER3 interrupt.
	EXTERNDEF timer3_int
