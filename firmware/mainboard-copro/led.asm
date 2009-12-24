	; asmsyntax=pic
	radix dec
	processor 18F4550
#include <p18f4550.inc>
#define IMPL
#include "led.inc"
#include "pins.inc"
#undefine IMPL




	
	udata
high_period: res 1
low_period: res 1
tick_count: res 1





	code
led_init:
	; Drive the output port low to turn the LED off.
	bcf LAT_LED, PIN_LED
	bcf TRIS_LED, PIN_LED

	; Configure and start the timer.
	movlw (1 << T1CKPS1) | (1 << T1CKPS0) | (1 << TMR1ON)
	movwf T1CON

	; Set the interrupt as low priority.
	bcf IPR1, TMR1IP

	; Do not enable the interrupt yet. Only enable it when blinking.
	return



led_on:
	; Turn off the timer interrupt. Only needed when blinking.
	bcf PIE1, TMR1IE

	; Drive the LED.
	bsf LAT_LED, PIN_LED

	; Done.
	return



led_off:
	; Turn off the timer interrupt. Only needed when blinking.
	bcf PIE1, TMR1IE

	; Turn off the LED.
	bcf LAT_LED, PIN_LED

	; Done.
	return



led_blink:
	; Turn off timer interrupt while manipulating registers.
	bcf PIE1, TMR1IE

	; Stash WREG in high_period temporarily.
	movff WREG, high_period

	; Grab only the low nybble (off period) and save it.
	andlw 0x0F
	movff WREG, low_period

	; Grab the high nybble (on period) and save it.
	movff high_period, WREG
	swapf WREG, W
	andlw 0x0F
	movff WREG, high_period

	; Clear the tick count.
	movlw 0
	movff WREG, tick_count

	; Turn the interrupt on so the LED starts blinking.
	bsf PIE1, TMR1IE

	; Done.
	return



timer1_int:
	; Clear interrupt flag.
	bcf PIR1, TMR1IF

	; Point at the variables.
	banksel tick_count

	; Check whether the LED is currently on or off.
	btfsc LAT_LED, PIN_LED
	bra timer1_int_led_on

	; LED is off. Check if current tick count equal to low period.
	movf tick_count, W
	cpfseq low_period
	bra timer1_int_nochange

	; LED needs to turn on.
	bsf LAT_LED, PIN_LED
	clrf tick_count
	return

timer1_int_led_on:
	; LED is on. Check if current tick count equal to high period.
	movf tick_count, W
	cpfseq high_period
	bra timer1_int_nochange

	; LED needs to turn off.
	bcf LAT_LED, PIN_LED
	clrf tick_count
	return

timer1_int_nochange:
	; Tick count is smaller than period. LED does not change state.
	incf tick_count, F
	return

	end
