	; asmsyntax=pic

	; led.asm
	; =======
	;
	; This file contains the code to manage the LED.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#define IMPL
#include "led.inc"
#undefine IMPL
#include "pins.inc"



	global tmr1if_handler



	udata
bsr_temp: res 1
int_bsr_temp: res 1
is_active: res 1



	code
led_init:
	; Save BSR.
	movff BSR, bsr_temp

	; Make timer 1 interrupts be low priority.
	bcf IPR1, TMR1IP

	; Turn off timer 1 interrupts.
	bcf PIE1, TMR1IE

	; Turn off CCP.
	clrf CCP2CON

	; Turn off LED.
	bcf LAT_LED, PIN_LED

	; Configure timer 1 with prescaling and turn it on.
	movlw (1 << T1CKPS1) | (1 << TMR1ON)
	movwf T1CON

	; Mark as not active.
	banksel is_active
	clrf is_active

	; Restore BSR.
	movff bsr_temp, BSR

	; Done.
	return



led_off:
	; Save BSR.
	movff BSR, bsr_temp

	; Turn off timer 1 interrupts.
	bcf PIE1, TMR1IE

	; Turn off CCP.
	clrf CCP2CON

	; Turn off LED.
	bcf LAT_LED, PIN_LED

	; Mark as not active.
	banksel is_active
	clrf is_active

	; Restore BSR.
	movff bsr_temp, BSR
	
	; Done.
	return



led_idle:
	; Enable timer 1 interrupts.
	bcf PIR1, TMR1IF
	bsf PIE1, TMR1IE

	; Do NOT touch the LED itself yet. If we did, we'd blink it far too fast in
	; a tight idle/active loop. Instead, when the timer interrupt fires, it will
	; turn on the LED and shut down the CCP. If led_activity is called really
	; soon, the interrupt will never fire and so the CCP will keep toggling the
	; LED at the right speed. If led_activity is not called soon, the timer
	; interrupt will fire and the LED will be forced back to solid. There's a
	; possible race, obviously, because maybe the timer is about to overflow and
	; will do so before we get to the next call to led_activity, thus killing
	; the CCP. But who cares: it's only an LED, and the next call to
	; led_activity will start things blinking again anyway!
	return



led_activity:
	; Save BSR.
	movff BSR, bsr_temp

	; Disable timer 1 interrupts.
	bcf PIE1, TMR1IE

	; Only do anything if not currently active.
	banksel is_active
	btfsc is_active, 0
	bra led_activity_already_active

	; Activate CCP2 in "compare and toggle" mode.
	bsf CCP2CON, CCP2M1

	; Mark active.
	setf is_active

led_activity_already_active:
	; Restore BSR.
	movff bsr_temp, BSR

	; Done.
	return



tmr1if_handler:
	; We got here because the LED is blinking but wants to be moved back into
	; the idle state, and it's our job to do that.
	;
	; Note that context saving is not needed here, because all we do is BCF and
	; BSF and those do not affect anything (including STATUS).

	; Save BSR.
	movff BSR, int_bsr_temp

	; Disable timer 1 interrupts.
	bcf PIE1, TMR1IE

	; Disable CCP2.
	bcf CCP2CON, CCP2M1

	; Turn the LED on solid.
	bsf LAT_LED, PIN_LED

	; Mark as inactive.
	banksel is_active
	clrf is_active

	; Restore BSR.
	movff int_bsr_temp, BSR

	; Done.
	retfie

	end
