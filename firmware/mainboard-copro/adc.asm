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



FLAG_CHICKER0 equ 2
FLAG_CHICKER110 equ 3
FLAG_CHICKER150 equ 4

ADC_ALIGN_LEFT equ 0
ADC_ALIGN_RIGHT equ 1

; Divider is 220k and 2.2k.
; Threshold for 200V: ADC reading = 200 / 222200 * 2200 / 3.3 * 1023 = 614
CHICKER200_THRESHOLD equ 614



	; Selects the specified ADC channel and performs a conversion.
ADC_CONVERT macro channel, align
if align == ADC_ALIGN_LEFT
	bcf ADCON2, ADFM
else
	bsf ADCON2, ADFM
endif
	movlw ((channel) << CHS0) | (1 << ADON)
	movwf ADCON0
	bsf ADCON0, GO
	btfsc ADCON0, GO
	bra $-2
	endm



	code
adc:
	; Drive the SPI bus.
	SPI_DRIVE
	
	; Turn on the ADC.
	movlw (1 << ADON)
	movwf ADCON0

	; Set acquisition time configuration.
	movlw (1 << ACQT1) | (1 << ADCS2) | (1 << ADCS0)
	movwf ADCON2

	; Go into a loop.
loop:
	; Check if the INIT signal line has gone low, meaning a post-configuration
	; CRC (if enabled) failed.
	btfss PORT_INIT_B, PIN_INIT_B
	reset

	; Lower /SS to the FPGA.
	bcf LAT_SPI_SS_FPGA, PIN_SPI_SS_FPGA

	; Read and send the battery level.
	ADC_CONVERT 0, ADC_ALIGN_RIGHT
	movf ADRESH, W
	SPI_SEND_WREG
	movf ADRESL, W
	SPI_SEND_WREG

	; Read, check, and send capacitor level.
	ADC_CONVERT 5, ADC_ALIGN_RIGHT
	movlw CHICKER200_THRESHOLD >> 2
	cpfslt ADRESH
	bra overcharge
	movf ADRESH, W
	SPI_SEND_WREG
	movf ADRESL, W
	SPI_SEND_WREG

	; Raise /SS.
	bsf LAT_SPI_SS_FPGA, PIN_SPI_SS_FPGA

	; Go back and do it again.
	bra loop



overcharge:
	; Capacitor dangerously overchaged and FPGA not doing anything about it.
	; Crash FPGA.
	bcf LAT_PROG_B, PIN_PROG_B
	; Turn on LED.
	bsf LAT_LED, PIN_LED
	; Die forever.
	clrf INTCON
	sleep
	bra overcharge
	end
