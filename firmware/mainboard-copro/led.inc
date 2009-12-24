	; asmsyntax=pic

#include "externdef.inc"

	; ======================
	; LED BLINKING FUNCTIONS
	; ======================
	;
	; These functions allow the diagnostic LED to be controlled. The LED is
	; timed by timer 1.



	; function led_init
	; =================
	;
	; Initializes the LED. The LED is initially off.
	EXTERNDEF led_init



	; function led_on
	; ===============
	;
	; Turns the LED on solid.
	EXTERNDEF led_on



	; function led_off
	; ================
	;
	; Turns the LED off solid.
	EXTERNDEF led_off



	; function led_blink
	; ==================
	;
	; Sets the LED blinking such that the LED is on for a period equal to
	; WREG<7:4> + 1 timer interrupts and then off for a period equal to
	; WREG<3:0> + 1 timer interrupts. A timer interrupt is approximately 131ms.
	EXTERNDEF led_blink



	; function timer1_int
	; ===================
	;
	; Must be invoked on a TIMER1 interrupt.
	EXTERNDEF timer1_int
