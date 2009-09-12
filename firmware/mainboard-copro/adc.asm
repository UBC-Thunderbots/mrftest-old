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
#include "pins.inc"
#include "spi.inc"



	global adc
	extern bootload



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
	movf current_value + 0, W
	call spi_transceive
	movf current_value + 1, W
	call spi_transceive
	endm



	; Waits for an in-progress conversion on (channel) to finish, then
	; simultaneously starts a conversion on (channel+1) and also sends the data
	; from (channel) out over SPI.
CONVERT_AND_SEND macro channel
	WAIT_ADC_FINISH
	ADC_START_CONVERSION (channel) + 1
	SEND_ADC_RESULT
	endm



	code
adc:
	; Let the motors run.
	bsf TRIS_BRAKE, PIN_BRAKE

	; Drive the SPI bus.
	call spi_drive

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

	; Check if the bootload signal line has gone high.
	btfsc PORT_XBEE_BL, PIN_XBEE_BL
	bra adc_done

	; Lower /SS to the FPGA.
	bcf LAT_SPI_SS_FPGA, PIN_SPI_SS_FPGA

	; Go through the thirteen channels, doing the overlapped convert-and-send.
	CONVERT_AND_SEND 0
	CONVERT_AND_SEND 1
	CONVERT_AND_SEND 2
	CONVERT_AND_SEND 3
	CONVERT_AND_SEND 4
	CONVERT_AND_SEND 5
	CONVERT_AND_SEND 6
	CONVERT_AND_SEND 7
	CONVERT_AND_SEND 8
	CONVERT_AND_SEND 9
	CONVERT_AND_SEND 10
	CONVERT_AND_SEND 11
	WAIT_ADC_FINISH
	SEND_ADC_RESULT

	; All thirteen channels have been sent. Raise /SS.
	bsf LAT_SPI_SS_FPGA, PIN_SPI_SS_FPGA

	; Go back and do it again.
	bra loop



adc_done:
	; The bootload signal was sent high.
	; Turn off the ADC.
	clrf ADCON0

	; Go into bootloading mode.
	goto bootload
	end
