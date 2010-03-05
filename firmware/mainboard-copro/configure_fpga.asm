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
#include "pins.inc"
#include "sleep.inc"



	global configure_fpga
	extern adc



	code
configure_fpga:
	; Enable mode-change interrupts.
	; We care about both signals (EMERG_ERASE=INT2, BOOTLOAD=INT1).
	; EMERG_ERASE should be high right now (deasserted).
	; BOOTLOAD should be low right now (deasserted).
	; A change on either one should reset the PIC.
	bcf INTCON2, INTEDG2
	bcf INTCON3, INT1IF
	bcf INTCON3, INT2IF
	bsf INTCON3, INT1IE
	bsf INTCON3, INT2IE

	; Check that we haven't raced and missed a change.
	btfss PORT_EMERG_ERASE, PIN_EMERG_ERASE
	reset
	btfsc PORT_XBEE_BL, PIN_XBEE_BL
	reset

	; Drive PROG_B high to begin the configuration process.
	bsf LAT_PROG_B, PIN_PROG_B

	; Once the FPGA is finished configuring itself, the DONE pin will go high.
	; Wait until that happens.
wait_for_done:
	btfss PORT_DONE, PIN_DONE
	bra wait_for_done

	; Wait a bit for stabilization.
	call sleep_1ms

	; The FPGA has now configured itself. Run as an ADC from now on.
	goto adc

	end
