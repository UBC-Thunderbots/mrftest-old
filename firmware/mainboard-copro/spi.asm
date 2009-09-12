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
	global spi_transceive



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



	; This macro transceives one bit of WREG.
	; The code below is carefully written to minimize pin transitions and
	; stabilize timing.
SPI_TRANSCEIVE_BIT macro BIT
	; Drive data line.
	btfsc WREG, BIT
	bsf LAT_SPI_TX, PIN_SPI_TX
	btfss WREG, BIT
	bcf LAT_SPI_TX, PIN_SPI_TX

	; While TX is stabilizing, sample RX.
	bcf WREG, BIT
	btfsc PORT_SPI_RX, PIN_SPI_RX
	bsf WREG, BIT

	; Raise clock line.
	bsf LAT_SPI_CK, PIN_SPI_CK

	; Wait 7 instruction cycles.
	bra $+2
	bra $+2
	bra $+2
	nop

	; Lower clock line.
	bcf LAT_SPI_CK, PIN_SPI_CK
	endm



spi_transceive:
	SPI_TRANSCEIVE_BIT 7
	SPI_TRANSCEIVE_BIT 6
	SPI_TRANSCEIVE_BIT 5
	SPI_TRANSCEIVE_BIT 4
	SPI_TRANSCEIVE_BIT 3
	SPI_TRANSCEIVE_BIT 2
	SPI_TRANSCEIVE_BIT 1
	SPI_TRANSCEIVE_BIT 0
	return
	end
