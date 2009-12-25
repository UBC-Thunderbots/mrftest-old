	; asmsyntax=pic

	; configure_fpga.asm
	; ==================
	;
	; This file contains the code to run the FPGA configuration sequence and get
	; the FPGA bootstrapped. After the FPGA is bootstrapped, the code jumps to
	; the ADC-and-XBee-monitor routine.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "led.inc"
#include "pins.inc"
#include "sleep.inc"
#include "spi.inc"



	global configure_fpga
	extern adc



	code
configure_fpga:
	; Drive PROG_B high to begin the configuration process.
	bsf LAT_PROG_B, PIN_PROG_B

	; While FPGA is configuring, blink LED at 1048ms period with 50% duty cycle.
	movlw (3 << 4) | 3
	call led_blink

	; Once the FPGA is finished configuring itself, the DONE pin will go high.
	; Wait until that happens. If the bootload pin is driven high during this
	; time, reset the PIC (also if the emergency erase pin is driven low).
wait_for_done:
	btfsc PORT_XBEE_BL, PIN_XBEE_BL
	reset
	btfss PORT_EMERG_ERASE, PIN_EMERG_ERASE
	reset
	btfss PORT_DONE, PIN_DONE
	bra wait_for_done

	; Wait a bit for stabilization.
	call sleep_1ms

	; The FPGA has now configured itself. Run as an ADC from now on.
	goto adc

	end
