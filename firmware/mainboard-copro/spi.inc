	; asmsyntax=pic

	; spi.inc
	; =======
	;
	; This file declares functions related to the SPI bus.
	;

#include "pins.inc"
#include "sleep.inc"



	; Macro SPI_DRIVE
	; ===============
	;
	; This drives the SPI bus (the bus is tristated at bootup).
	;
SPI_DRIVE macro
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

	; Wait for everything to settle.
	call sleep_10ms
	endm



	; Macro SPI_SEND_BIT_WREG
	; =======================
	;
	; This sends one bit of WREG. It is generally not used directly; instead,
	; SPI_SEND_WREG is used.
	;
SPI_SEND_BIT_WREG macro bit
	; Drive data line.
	btfsc WREG, bit
	bsf LAT_SPI_TX, PIN_SPI_TX
	btfss WREG, bit
	bcf LAT_SPI_TX, PIN_SPI_TX

	; Raise clock line.
	bsf LAT_SPI_CK, PIN_SPI_CK

	; Lower clock line.
	bcf LAT_SPI_CK, PIN_SPI_CK
	endm



	; Macro SPI_SEND_WREG
	; ===================
	;
	; This sends all eight bits of WREG out over SPI.
	;
SPI_SEND_WREG macro
	SPI_SEND_BIT_WREG 7
	SPI_SEND_BIT_WREG 6
	SPI_SEND_BIT_WREG 5
	SPI_SEND_BIT_WREG 4
	SPI_SEND_BIT_WREG 3
	SPI_SEND_BIT_WREG 2
	SPI_SEND_BIT_WREG 1
	SPI_SEND_BIT_WREG 0
	endm



	; Macro SPI_SEND_BIT_CONSTANT
	; ===========================
	;
	; This sends one bit of a constant byte. It is generally not used directly;
	; instead, SPI_SEND_CONSTANT is used.
	;
SPI_SEND_BIT_CONSTANT macro byte, bit
	; Drive data line.
	if (byte & (1 << bit)) != 0
		bsf LAT_SPI_TX, PIN_SPI_TX
	else
		bcf LAT_SPI_TX, PIN_SPI_TX
	endif

	; Raise clock line.
	bsf LAT_SPI_CK, PIN_SPI_CK

	; Lower clock line.
	bcf LAT_SPI_CK, PIN_SPI_CK
	endm



	; Macro SPI_SEND_CONSTANT
	; =======================
	;
	; This sends all eight bits of a constant byte value out over SPI. It can
	; only be used to send numerical constants, but is faster than
	; SPI_SEND_WREG.
	;
SPI_SEND_CONSTANT macro byte
	SPI_SEND_BIT_CONSTANT byte, 7
	SPI_SEND_BIT_CONSTANT byte, 6
	SPI_SEND_BIT_CONSTANT byte, 5
	SPI_SEND_BIT_CONSTANT byte, 4
	SPI_SEND_BIT_CONSTANT byte, 3
	SPI_SEND_BIT_CONSTANT byte, 2
	SPI_SEND_BIT_CONSTANT byte, 1
	SPI_SEND_BIT_CONSTANT byte, 0
	endm



	; Macro SPI_RECEIVE_BIT
	; =====================
	;
	; This receives one bit of data. It is generally not used directly; instead,
	; SPI_RECEIVE_WREG is used.
SPI_RECEIVE_BIT macro dest, bit
	; Raise clock line.
	bsf LAT_SPI_CK, PIN_SPI_CK

	; Sample receive line.
	btfsc PORT_SPI_RX, PIN_SPI_RX
	bsf dest, bit

	; Lower clock line.
	bcf LAT_SPI_CK, PIN_SPI_CK
	endm



	; Macro SPI_RECEIVE
	; =================
	;
	; This receives eight bits from SPI and stores them into dest.
	;
SPI_RECEIVE macro dest
	clrf dest
	SPI_RECEIVE_BIT dest, 7
	SPI_RECEIVE_BIT dest, 6
	SPI_RECEIVE_BIT dest, 5
	SPI_RECEIVE_BIT dest, 4
	SPI_RECEIVE_BIT dest, 3
	SPI_RECEIVE_BIT dest, 2
	SPI_RECEIVE_BIT dest, 1
	SPI_RECEIVE_BIT dest, 0
	endm

