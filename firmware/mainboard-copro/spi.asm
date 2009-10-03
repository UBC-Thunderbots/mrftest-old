	; asmsyntax=pic

	; spi.asm
	; =======
	;
	; This file includes definitions of the functions declared in spi.inc.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "pins.inc"
#include "sleep.inc"



	global spi_drive
	global spi_tristate
	global spi_send
	global spi_receive



	code
spi_drive:
	; First, drive both SLAVE SELECT lines high.
	bsf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	bcf TRIS_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	bsf LAT_SPI_SS_FPGA, PIN_SPI_SS_FPGA
	bcf TRIS_SPI_SS_FPGA, PIN_SPI_SS_FPGA

	; Drive the CLOCK line low.
	bcf LAT_SPI_CK, PIN_SPI_CK
	bcf TRIS_SPI_CK, PIN_SPI_CK

	; Drive the TRANSMIT line low.
	bcf LAT_SPI_TX, PIN_SPI_TX
	bcf TRIS_SPI_TX, PIN_SPI_TX

	; Done!
	return



spi_tristate:
	; Tristate CLOCK and TRANSMIT first.
	bsf TRIS_SPI_CK, PIN_SPI_CK
	bsf TRIS_SPI_TX, PIN_SPI_TX

	; Now tristate both SLAVE SELECT lines.
	bsf TRIS_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	bsf TRIS_SPI_SS_FPGA, PIN_SPI_SS_FPGA

	; Done!
	return



	; This macro sends one bit of WREG.
	; The code below is carefully written to minimize pin transitions and
	; stabilize timing.
SPI_SEND_BIT macro bit
	; Drive data line.
	btfsc WREG, bit
	bsf LAT_SPI_TX, PIN_SPI_TX
	btfss WREG, bit
	bcf LAT_SPI_TX, PIN_SPI_TX

	; Raise clock line.
	bsf LAT_SPI_CK, PIN_SPI_CK

	; Wait 4 instruction cycles.
	bra $+2
	bra $+2

	; Lower clock line.
	bcf LAT_SPI_CK, PIN_SPI_CK
	endm



spi_send:
	SPI_SEND_BIT 7
	SPI_SEND_BIT 6
	SPI_SEND_BIT 5
	SPI_SEND_BIT 4
	SPI_SEND_BIT 3
	SPI_SEND_BIT 2
	SPI_SEND_BIT 1
	SPI_SEND_BIT 0
	bcf LAT_SPI_TX, PIN_SPI_TX
	return



	; This macro receives one bit of WREG.
	; The code below is carefully written to minimize pin transitions and
	; stabilize timing.
SPI_RECEIVE_BIT macro bit
	; Raise clock line.
	bsf LAT_SPI_CK, PIN_SPI_CK

	; Sample receive line.
	btfsc LAT_SPI_RX, PIN_SPI_RX
	iorlw 1 << bit

	; Lower clock line.
	bcf LAT_SPI_CK, PIN_SPI_CK
	endm



spi_receive:
	movlw 0
	SPI_RECEIVE_BIT 7
	SPI_RECEIVE_BIT 6
	SPI_RECEIVE_BIT 5
	SPI_RECEIVE_BIT 4
	SPI_RECEIVE_BIT 3
	SPI_RECEIVE_BIT 2
	SPI_RECEIVE_BIT 1
	SPI_RECEIVE_BIT 0
	return
	end
