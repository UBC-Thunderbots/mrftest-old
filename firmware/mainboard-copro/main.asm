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
#include "led.inc"
#include "pins.inc"
#include "sleep.inc"



	extern emergency_erase
	extern bootload
	extern configure_fpga
	extern rcif_handler
	extern tmr1if_handler



resetvec code
	; This code is burned at address 0x800, where the PIC starts running after
	; the boot block.
	goto main



intvechigh code
	; This code is burned at address 0x808, where high priority interrupts go
	; after the boot block.
	btfsc PIR1, RCIF
	goto rcif_handler
	btfsc INTCON3, INT1IF
	reset
	btfsc INTCON3, INT2IF
	reset
	retfie FAST



intveclow code
	; This code is burned at address 0x818, where low priority interrupts go
	; after the boot block.
	btfsc PIR1, TMR1IF
	goto tmr1if_handler
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

	; The ADC initializes AN0..7 as analogue inputs by default. We actually only
	; want AN0..5.
	movlw (1 << PCFG3) | (1 << PCFG0)
	movwf ADCON1

	; Initialize the I/O pins.
	PINS_INITIALIZE

	; Enable the LED module.
	call led_init

	; Configure the SLEEP instruction to go into IDLE mode, and the internal
	; oscillator to run at 8MHz.
	movlw (1 << IDLEN) | (1 << IRCF2) | (1 << IRCF1) | (1 << IRCF0)
	movwf OSCCON

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
	; Before doing this, reset the XBee (just in case). We don't want to do this
	; if the emergency erase or bootload line is set, because the line would be
	; dropped by the reset.
	bcf LAT_XBEE_RESET, PIN_XBEE_RESET
	call sleep_10ms
	bsf LAT_XBEE_RESET, PIN_XBEE_RESET
	call sleep_100ms
	call sleep_100ms
	goto configure_fpga
	end
