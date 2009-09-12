	; asmsyntax=pic

	; main.asm
	; ========
	;
	; This file contains the application entry point, where code starts running.
	; The code in this file initializes the I/O pins and then jumps to the FPGA
	; configuration routine or the bootloader depending on the state of the XBee
	; signal line.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "pins.inc"



	extern configure_fpga
	extern bootload



resetvec code
	; This code is burned at address 0, where the PIC starts running.
	goto main



	code
main:
	; Initialize those pins that should be outputs to safe initial levels.
	; Pins are configured as inputs at device startup.
	; DONE is an input read from the FPGA.
	; PROG_B goes low to hold the FPGA in pre-configuration until ready.
	bcf LAT_PROG_B, PIN_PROG_B
	bcf TRIS_PROG_B, PIN_PROG_B
	; INIT_B is an input read from the FPGA.
	; BRAKE goes low to lock the wheels until the FPGA is running.
	bcf LAT_BRAKE, PIN_BRAKE
	bcf TRIS_BRAKE, PIN_BRAKE
	; SPI_TX is tristated while the FPGA drives it to load from Flash.
	; SPI_RX is always an input.
	; SPI_CK is tristated while the FPGA drives it to load from Flash.
	; SPI_SPI_SS_FLASH is tristated while the FPGA drives it to load from Flash.
	; SPI_SPI_SS_FPGA is high until the ADC is ready to send data to the FPGA.
	bsf LAT_SPI_SS_FPGA, PIN_SPI_SS_FPGA
	bcf TRIS_SPI_SS_FPGA, PIN_SPI_SS_FPGA
	; FLASH_WP is high unless bootloading.
	bsf LAT_FLASH_WP, PIN_FLASH_WP
	bcf TRIS_FLASH_WP, PIN_FLASH_WP
	; XBEE_TX is tristated unless in bootloader mode.
	; XBEE_RX is always an input.
	; XBEE_BL is always an input.
	; ICSP_PGD is always an input.
	; ICSP_PGC is always an input.
	; ICSP_PGM is always an input.
	; USB_DP is low until controlled by the SIE.
	bcf LAT_USB_DP, PIN_USB_DP
	bcf TRIS_USB_DP, PIN_USB_DP
	; USB_DM is low until controlled by the SIE.
	bcf LAT_USB_DM, PIN_USB_DM
	bcf TRIS_USB_DM, PIN_USB_DM
	; Non-connected pins are externally grounded. Leave them as inputs.

	; Now that we've initialized ourself, we either go into bootloader mode or
	; go into FPGA configuration mode, depending on the state of the XBee pin.
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	goto configure_fpga
	goto bootload

	end
