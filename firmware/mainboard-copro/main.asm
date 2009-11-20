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
#include "dbgprint.inc"
#include "pins.inc"
#include "sleep.inc"



	extern configure_fpga
	extern bootload
	extern rcif_main, txif_main



	udata
intlow_status: res 1
intlow_bsr: res 1
intlow_wreg: res 1



resetvec code
	; This code is burned at address 0, where the PIC starts running.
	goto main



intvechigh code
	; This code is burned at address 8, where high priority interrupts go.
	call timer3_int
	retfie FAST



intveclow code
	; This code is burned at address 18, where low priority interrupts go.
	movff STATUS, intlow_status
	movff BSR, intlow_bsr
	banksel intlow_wreg
	movwf intlow_wreg

	btfsc PIR1, RCIF
	call rcif_main
	btfsc PIR1, TXIF
	call txif_main

	banksel intlow_wreg
	movf intlow_wreg, W
	movff intlow_bsr, BSR
	movff intlow_status, STATUS
	retfie



	code
main:
	; Enable global interrupts and interrupt priorities. Do not enable any
	; particular interrupts.
	bsf RCON, IPEN
	movlw (1 << GIEL) | (1 << GIEH)
	movwf INTCON

	; Initialize those pins that should be outputs to safe initial levels.
	; Pins are configured as inputs at device startup.
	; DONE is an input read from the FPGA.
	; PROG_B goes low to hold the FPGA in pre-configuration until ready.
	bcf LAT_PROG_B, PIN_PROG_B
	bcf TRIS_PROG_B, PIN_PROG_B
	; INIT_B is an input read from the FPGA.
	; BRAKE goes high to lock the wheels until the FPGA is running.
	bsf LAT_BRAKE, PIN_BRAKE
	bcf TRIS_BRAKE, PIN_BRAKE
	; SPI_TX is tristated if FPGA will configure itself, until in run mode.
	; If in bootload mode, bootloader will drive SPI_TX.
	; SPI_RX is always an input.
	; SPI_CK is tristated (see SPI_TX).
	; SPI_SPI_SS_FLASH is tristated (see SPI_TX).
	; SPI_SPI_SS_FPGA is high until the ADC is ready to send data to the FPGA.
	bsf LAT_SPI_SS_FPGA, PIN_SPI_SS_FPGA
	bcf TRIS_SPI_SS_FPGA, PIN_SPI_SS_FPGA
	; FLASH_WP is high unless bootloading.
	bsf LAT_FLASH_WP, PIN_FLASH_WP
	bcf TRIS_FLASH_WP, PIN_FLASH_WP
	; XBEE_TX is tristated unless bootloading.
	; XBEE_RX is always an input.
	; XBEE_BL is always an input.
	; ICSP_PGD is managed by DBGPRINT.
	; ICSP_PGC is always an input.
	; ICSP_PGM is always an input.
	; USB_DP is low to avoid floating pin.
	bcf LAT_USB_DP, PIN_USB_DP
	bcf TRIS_USB_DP, PIN_USB_DP
	; USB_DM is low to avoid floating pin.
	bcf LAT_USB_DM, PIN_USB_DM
	bcf TRIS_USB_DM, PIN_USB_DM
	; RTS is low until used in bootloader for flow control.
	bcf LAT_RTS, PIN_RTS
	bcf TRIS_RTS, PIN_RTS

	; Initialize the debugging library.
	call dbgprint_init

	; Wait a tenth of a second for everything to stabilize.
	call sleep_100ms

	; Now that we've initialized ourself, we either go into bootloader mode or
	; go into FPGA configuration mode, depending on the state of the XBee pin.
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	goto configure_fpga
	goto bootload
	end
