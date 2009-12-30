	; asmsyntax=pic

	; adc.asm
	; =======
	;
	; The code in this file runs the PIC as an analogue-to-digital converter,
	; which happens while the FPGA is up and running.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "led.inc"
#include "pins.inc"
#include "spi.inc"



	global adc



	udata
current_value: res 2



	; Selects the specified ADC channel and starts a conversion.
ADC_START_CONVERSION macro channel
	movlw ((channel) << CHS0) | (1 << ADON)
	movwf ADCON0
	bsf ADCON0, GO
	endm



	; Waits for an in-progress ADC conversion to finish.
WAIT_ADC_FINISH macro
	btfsc ADCON0, GO
	bra $-2
	movff ADRESL, current_value + 0
	movff ADRESH, current_value + 1
	endm



	; Sends the result of an ADC conversion out over SPI.
SEND_ADC_RESULT macro
	movf current_value + 1, W
	call spi_send
	movf current_value + 0, W
	call spi_send
	endm



	; Waits for an in-progress conversion on (oldchannel) to finish, then
	; simultaneously starts a conversion on (newchannel) and also sends the data
	; from (oldchannel) out over SPI.
CONVERT_AND_SEND macro oldchannel, newchannel
	WAIT_ADC_FINISH
	ADC_START_CONVERSION (newchannel)
	SEND_ADC_RESULT
	endm



	code
adc:
	; Turn the LED on solid.
	call led_on

	; Let the motors run.
	bsf TRIS_BRAKE, PIN_BRAKE

	; Drive the SPI bus.
	call spi_drive
	
	; Select proper bank.
	banksel current_value

	; Turn on the ADC.
	movlw (1 << ADON)
	movwf ADCON0

	; Set acquisition time configuration.
	movlw (1 << ADFM) | (1 << ACQT1) | (1 << ADCS1)
	movwf ADCON2

	; Go into a loop.
loop:
	; Select ADC channel zero and begin an acquisition.
	ADC_START_CONVERSION 0

	; Check if the bootload signal line has been asserted (gone high).
	btfsc PORT_XBEE_BL, PIN_XBEE_BL
	reset

	; Check if the emergency erase signal line has been asserted (gone low).
	btfss PORT_EMERG_ERASE, PIN_EMERG_ERASE
	reset

	; Check if the DONE signal line has been deasserted (gone low), meaning a 
	; post-configuration CRC (if enabled) failed.
	btfss PORT_DONE, PIN_DONE
	reset

	; Lower /SS to the FPGA.
	bcf LAT_SPI_SS_FPGA, PIN_SPI_SS_FPGA

	; Go through the thirteen channels, doing the overlapped convert-and-send.
	CONVERT_AND_SEND 0, 1
	CONVERT_AND_SEND 1, 2
	CONVERT_AND_SEND 2, 3
	CONVERT_AND_SEND 3, 4
	CONVERT_AND_SEND 4, 5
	CONVERT_AND_SEND 5, 6
	CONVERT_AND_SEND 6, 7
	CONVERT_AND_SEND 7, 11 ; NOTE: To ease board layout, PIC channels
	CONVERT_AND_SEND 11, 9 ; DO NOT directly correspond to logical channels.
	CONVERT_AND_SEND 9, 8  ; Check the schematic for the exact mapping.
	CONVERT_AND_SEND 8, 10
	CONVERT_AND_SEND 10, 12
	WAIT_ADC_FINISH
	SEND_ADC_RESULT

	; All thirteen channels have been sent. Raise /SS.
	bsf LAT_SPI_SS_FPGA, PIN_SPI_SS_FPGA

	; Go back and do it again.
	bra loop
	end
