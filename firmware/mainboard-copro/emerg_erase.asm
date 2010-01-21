	; asmsyntax=pic

	; emerg_erase.asm
	; ===============
	;
	; This fiel contains the code to erase the Flash chip when instructed to do
	; so by the "Emergency Erase" pin, which is triggered either by the XBee or
	; by a manually-applied short circuit on a testpoint.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "pins.inc"
#include "spi.inc"



	global emergency_erase



	code
	; Main code.
emergency_erase:
	; Take control of the SPI bus.
	SPI_DRIVE

	; Allow writes to the Flash chip.
	bcf LAT_FLASH_WP, PIN_FLASH_WP

	; Wait a while for everything to settle.
	call sleep_100ms

	; Send WRITE ENABLE.
	rcall select_chip
	SPI_SEND_CONSTANT 0x06
	rcall deselect_chip

	; Send CHIP ERASE.
	rcall select_chip
	SPI_SEND_CONSTANT 0xC7
	rcall deselect_chip

	; Send READ STATUS REGISTER and wait until not BUSY.
	rcall select_chip
	SPI_SEND_CONSTANT 0x05
wait_nonbusy:
	SPI_RECEIVE WREG
	btfsc WREG, 0
	bra wait_nonbusy

	; Wait until the emergency erase signal line is deasserted (goes high).
wait_pin:
	btfss PORT_EMERG_ERASE, PIN_EMERG_ERASE
	bra wait_pin

	reset



select_chip:
	rcall sleep_1us
	bcf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	rcall sleep_1us
	return



deselect_chip:
	rcall sleep_1us
	bsf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	rcall sleep_1us
	return

	end
