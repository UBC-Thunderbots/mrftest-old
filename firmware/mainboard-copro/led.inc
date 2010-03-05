	; asmsyntax=pic

	; led.inc
	; =======
	;
	; The functions in this file allow controlling the status LED.
	;

#include "externdef.inc"

	; Function led_init
	; =================
	;
	; This function initializes the timer and CCP to operate the LED, leaving
	; the LED turned off.
	;
	EXTERNDEF led_init

	; Function led_off
	; ================
	;
	; This function turns the LED off.
	;
	EXTERNDEF led_off

	; Function led_idle
	; =================
	;
	; This function turns the LED on solid, as though idle (interacting properly
	; with led_activity to give a nice effect).
	;
	EXTERNDEF led_idle

	; Function led_activity
	; =====================
	;
	; This function starts the LED blinking, with the property that it will keep
	; blinking until led_idle or led_off is called, but that if led_idle and
	; led_activity are called repeatedly in rapid succession, the LED will still
	; blink visibly.
	;
	EXTERNDEF led_activity

