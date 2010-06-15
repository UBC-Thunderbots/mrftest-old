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
; Threshold for 0 is 10V: ADC reading = 10 / 222200 * 2200 / 3.3 * 1023 = 31
; Threshold for 110V: ADC reading = 110 / 222200 * 2200 / 3.3 * 1023 = 338
; Threshold for 150V: ADC reading = 150 / 222200 * 2200 / 3.3 * 1023 = 460
; Threshold for 200V: ADC reading = 200 / 222200 * 2200 / 3.3 * 1023 = 614
CHICKER0_THRESHOLD equ 31
CHICKER110_THRESHOLD equ 338
CHICKER150_THRESHOLD equ 460
CHICKER200_THRESHOLD equ 614



	udata
current_value: res 2



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
	
	; Select proper bank.
	banksel current_value

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

	; Read the battery level.
	ADC_CONVERT 0, ADC_ALIGN_RIGHT
	movff ADRESH, current_value + 1
	movff ADRESL, current_value + 0

	; Read and check the capacitor level.
	ADC_CONVERT 5, ADC_ALIGN_LEFT
	movlw CHICKER0_THRESHOLD >> 2
	cpfsgt ADRESH
	bsf current_value + 1, FLAG_CHICKER0
	movlw CHICKER110_THRESHOLD >> 2
	cpfslt ADRESH
	bsf current_value + 1, FLAG_CHICKER110
	movlw CHICKER150_THRESHOLD >> 2
	cpfslt ADRESH
	bsf current_value + 1, FLAG_CHICKER150
	movlw CHICKER200_THRESHOLD >> 2
	cpfslt ADRESH
	bra overcharge

	; Send the data.
	movf current_value + 1, W
	SPI_SEND_WREG
	movf current_value + 0, W
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
