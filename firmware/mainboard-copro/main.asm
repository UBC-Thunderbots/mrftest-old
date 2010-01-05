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
#include "led.inc"
#include "pins.inc"
#include "sleep.inc"



	extern emergency_erase
	extern bootload
	extern configure_fpga



	udata
inthigh_tblptr: res 2
inthigh_tablat: res 1
intlow_status: res 1
intlow_wreg: res 1
intlow_bsr: res 1



resetvec code
	; This code is burned at address 0, where the PIC starts running.
	goto main



intvechigh code
	; This code is burned at address 8, where high priority interrupts go.
	; Workaround for a silicon bug.
	call intvechigh_impl, FAST



intvechigh_impl_seg code
intvechigh_impl:
	; Workaround for a silicon bug.
	pop

	movff TBLPTRL, inthigh_tblptr + 0
	movff TBLPTRH, inthigh_tblptr + 1
	movff TABLAT, inthigh_tablat

	call timer3_int

	movff inthigh_tablat, TABLAT
	movff inthigh_tblptr + 1, TBLPTRH
	movff inthigh_tblptr + 0, TBLPTRL
	retfie FAST



intveclow code
	; This code is burned at address 0x18, where low priority interrupts go.
	movff STATUS, intlow_status
	movff WREG, intlow_wreg
	movff BSR, intlow_bsr

	call timer1_int

	movff intlow_bsr, BSR
	movff intlow_wreg, WREG
	movff intlow_status, STATUS
	retfie



	code
main:
	; Enable global interrupts and interrupt priorities. Do not enable any
	; particular interrupts.
	bsf RCON, IPEN
	movlw (1 << GIEL) | (1 << GIEH)
	movwf INTCON

	; USB transceiver must be disabled to use RC4/RC5 as digital inputs.
	bsf UCFG, UTRDIS

	; Initialize those pins that should be outputs to safe initial levels.
	; Pins are configured as inputs at device startup.
	; DONE is an input read from the FPGA.
	; PROG_B goes low to hold the FPGA in pre-configuration until ready.
	bcf LAT_PROG_B, PIN_PROG_B
	bcf TRIS_PROG_B, PIN_PROG_B
	; INIT_B is an input read from the FPGA.
	; UNUSED goes low to avoid floating inputs.
	bcf LAT_UNUSED, PIN_UNUSED
	bcf TRIS_UNUSED, PIN_UNUSED
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
	; EMERG_ERASE is always an input.
	; LED is managed by LED.
	; RTS is low until used in bootloader for flow control.
	bcf LAT_RTS, PIN_RTS
	bcf TRIS_RTS, PIN_RTS

	; Initialize the blinky light library.
	call led_init

	; Initialize the debugging library.
	call dbgprint_init

	; Wait a tenth of a second for everything to stabilize.
	call sleep_100ms

	; Now that we've initialized ourself, we either go into emergency erase
	; mode, bootloader mode, or FPGA configuration mode, depending on the states
	; of the XBee pins.

	; EMERG_ERASE is active low. If low, do emergency erase.
	btfss PORT_EMERG_ERASE, PIN_EMERG_ERASE
	goto emergency_erase

	; BOOTLOAD is active high. If high, do bootload.
	btfsc PORT_XBEE_BL, PIN_XBEE_BL
	goto bootload

	; Otherwise, configure the FPGA and then become an ADC.
	goto configure_fpga
	end
