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
#include "led.inc"
#include "pins.inc"
#include "sleep.inc"
#include "spi.inc"



	global emergency_erase



	code
	; Main code.
emergency_erase:
	; Take control of the SPI bus.
	call spi_drive

	; Allow writes to the Flash chip.
	bcf LAT_FLASH_WP, PIN_FLASH_WP

	; Wait a while for everything to settle.
	call sleep_100ms

	; Send WRITE ENABLE.
	rcall select_chip
	movlw 0x06
	call spi_send
	rcall deselect_chip

	; Send CHIP ERASE.
	rcall select_chip
	movlw 0xC7
	call spi_send
	rcall deselect_chip

	; Blink the LED slowly at 50% duty cycle while doing an emergency erase.
	movlw (8 << 4) | 8
	call led_blink

	; Send READ STATUS REGISTER and wait until not BUSY.
	rcall select_chip
	movlw 0x05
	call spi_send
wait_nonbusy:
	call spi_receive
	btfsc WREG, 0
	bra wait_nonbusy

	; Hold the LED off once emergency erase is done.
	call led_off

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
