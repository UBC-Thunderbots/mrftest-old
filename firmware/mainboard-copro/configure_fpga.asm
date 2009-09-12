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
#include "spi.inc"



	global configure_fpga
	extern adc
	extern bootload



	code
configure_fpga:
	; Lock the wheels.
	bcf LAT_BRAKE, PIN_BRAKE

	; Tristate the SPI bus so that the FPGA can drive it.
	call spi_tristate

	; Drive PROG_B low to send the FPGA into configuration mode.
	bcf LAT_PROG_B, PIN_PROG_B

	; Wait a tenth of a second for everything to stabilize.
	call sleep_100ms

	; Drive PROG_B high to begin the configuration process.
	bsf LAT_PROG_B, PIN_PROG_B

	; Once the FPGA is finished configuring itself, the DONE pin will go high.
	; Wait until that happens. If the bootload pin is driven high during this
	; time, go over to the bootloader instead.
wait_for_done:
	btfsc PORT_XBEE_BL, PIN_XBEE_BL
	goto bootload
	btfss PORT_DONE, PIN_DONE
	bra wait_for_done

	; The FPGA has now configured itself. Run as an ADC from now on.
	goto adc

	end
