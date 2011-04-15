#include "channels.h"
#include <pic18fregs.h>

__code __at(0x7800) uint8_t channels[2] = { 0x0E, 0x0F };

void channels_change(uint8_t channel0, uint8_t channel1) {
	uint8_t old_intcon;
	static uint8_t s_channel0, s_channel1;
	s_channel0 = channel0;
	s_channel1 = channel1;
	old_intcon = INTCON;
	INTCONbits.GIEH = 0;
	__asm
		; Erase the data.
		movlw LOW(_channels)
		movwf _TBLPTRL
		movlw HIGH(_channels)
		movwf _TBLPTRH
		movlw UPPER(_channels)
		movwf _TBLPTRU
		;              //------- Unimplemented
		;              ||/------ Deal with 64 bits at a time
		;              |||/----- Erase
		;              ||||/---- No error
		;              |||||/--- Enable write
		;              ||||||/-- Do not start writing yet
		;              |||||||/- Unimplemented
		movlw 0x14 ; 0b00010100
		movwf _EECON1
		movlw 0x55
		movwf _EECON2
		movlw 0xAA
		movwf _EECON2
		bsf _EECON1, 1 ; Start write

		; Write the new data.
		movff _channels_change_s_channel0_1_1, _TABLAT
		tblwt *+
		movff _channels_change_s_channel1_1_1, _TABLAT
		tblwt *+
		;              //------- Unimplemented
		;              ||/------ Deal with 64 bits at a time
		;              |||/----- Do not erase
		;              ||||/---- No error
		;              |||||/--- Enable write
		;              ||||||/-- Do not start writing yet
		;              |||||||/- Unimplemented
		movlw 0x04 ; 0b00000100
		movwf _EECON1
		movlw 0x55
		movwf _EECON2
		movlw 0xAA
		movwf _EECON2
		bsf _EECON1, 1 ; Start write
	__endasm;
	INTCON = old_intcon;
}

