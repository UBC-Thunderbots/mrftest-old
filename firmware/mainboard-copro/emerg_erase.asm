	; asmsyntax=pic

	; emerg_erase.asm
	; ===============
	;
	; This file contains the code to erase the Flash chip when instructed to do
	; so by the "Emergency Erase" pin, which is triggered either by the XBee or
	; by a manually-applied short circuit on a testpoint.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "led.inc"
#include "pins.inc"
#include "spi.inc"



	global emergency_erase



	; Asserts the SLAVE SELECT line to the Flash.
SELECT_CHIP macro
	bcf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	endm

	; Deasserts the SLAVE SELECT line to the Flash.
DESELECT_CHIP macro
	bsf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	endm



	code
	; Main code.
emergency_erase:
	; In emergency erase mode, we want to perform the operation and not let
	; anything get in our way. The only way to escape should be to cut power to
	; the board. Thus, do NOT enable mode-change interrupts!

	; Blink the LED while erasing.
	call led_activity

	; Take control of the SPI bus.
	SPI_DRIVE

	; Wait a while for everything to settle.
	call sleep_100ms

	; Send WRITE ENABLE.
	SELECT_CHIP
	SPI_SEND_CONSTANT 0x06
	DESELECT_CHIP

	; Send CHIP ERASE.
	SELECT_CHIP
	SPI_SEND_CONSTANT 0xC7
	DESELECT_CHIP

	; Send READ STATUS REGISTER and wait until not BUSY.
	SELECT_CHIP
	SPI_SEND_CONSTANT 0x05
wait_nonbusy:
	SPI_RECEIVE WREG
	btfsc WREG, 0
	bra wait_nonbusy

	; Done erasing. Occult the LED until powered down.
	clrf CCP2CON
occult_loop:
	bsf LAT_LED, PIN_LED
	call sleep_100ms
	call sleep_100ms
	call sleep_100ms
	call sleep_100ms
	call sleep_100ms
	call sleep_100ms
	call sleep_100ms
	call sleep_100ms
	call sleep_100ms
	bcf LAT_LED, PIN_LED
	call sleep_100ms
	call sleep_100ms
	bra occult_loop

	end
