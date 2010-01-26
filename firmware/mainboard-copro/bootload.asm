	; asmsyntax=pic

	; bootload.asm
	; ============
	;
	; This file contains the code to talk to the XBee when in bootloader mode.
	; This allows the FPGA configuration bitstream to be updated over the radio.
	;
	; The general structure of the communication protocol is as follows:
	;
	; The host sends over the radio a COMMAND. The COMMAND packet has this form:
	;  1 byte command ID
	;  2 bytes page number
	;  n bytes command data
	;
	; The bootloader receives the command, handles it, and sends back a
	; RESPONSE. The RESPONSE packet has this form:
	;  1 byte command ID
	;    This byte has the same value as the corresponding byte in the command
	;    packet.
	;  1 byte command status
	;    This byte contains one of the COMMAND_STATUS_* constants.
	;  n bytes response data (only if status is COMMAND_STATUS_OK)
	;
	; The host is expected to only send another command after receiving the
	; previous command's response.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "cbuf.inc"
#include "dispatch.inc"
#include "pins.inc"
#include "spi.inc"



	global bootload
	global rcif_handler



	; COMMAND_IDENT
	; =============
	;
	; Requests an ID string from the bootloader and Flash chip. This command can
	; be submitted at any time; it does not actually talk to the Flash chip (the
	; JEDEC ID is cached at bootloader initialization).
	;
	; Page number:
	;  Ignored
	;
	; Request data:
	;  None
	;
	; Response data:
	;  5 bytes ASCII string 'TBOTS'
	;  1 byte JEDEC manufacturer ID of Flash chip
	;  1 byte JEDEC memory type ID of Flash chip
	;  1 byte JEDEC capacity ID of Flash chip
	;
COMMAND_IDENT equ 0x1

	; COMMAND_ERASE_BLOCK
	; ===================
	;
	; Erases a block.
	;
	; Page number:
	;  The page number of the first page in the block to erase
	;
	; Request data:
	;  None
	;
	; Response data:
	;  None
	;
COMMAND_ERASE_BLOCK equ 0x2

	; COMMAND_WRITE_PAGE1
	; ===================
	;
	; Begins a write operation.
	;
	; Page number:
	;  The page number of the page to write
	;
	; Request data:
	;  86 bytes data to burn
	;
	; Response data:
	;  None
	;
COMMAND_WRITE_PAGE1 equ 0x3

	; COMMAND_WRITE_PAGE2
	; ===================
	;
	; Continues a write operation started with COMMAND_WRITE_PAGE1.
	;
	; Page number:
	;  Ignored
	;
	; Request data:
	;  86 bytes data to burn
	;
	; Response data:
	;  None
	;
COMMAND_WRITE_PAGE2 equ 0x4

	; COMMAND_WRITE_PAGE3
	; ===================
	;
	; Finishes a write operation continued with COMMAND_WRITE_PAGE2.
	;
	; Page number:
	;  Ignored
	;
	; Request data:
	;  84 bytes data to burn
	;
	; Response data:
	;  None
	;
COMMAND_WRITE_PAGE3 equ 0x5

	; COMMAND_CRC_SECTOR
	; ==================
	;
	; Performs a CRC16 of a sector.
	;
	; Page number:
	;  The page number of the first page in the sector to CRC.
	;
	; Request data:
	;  None
	;
	; Response data:
	;  32 bytes CRC16s of the pages
	;
COMMAND_CRC_SECTOR equ 0x6

	; COMMAND_ERASE_SECTOR
	; ====================
	;
	; Erases a sector.
	;
	; Page number:
	;  The page number of the first page in the sector to erase
	;
	; Request data:
	;  None
	;
	; Response data:
	;  None
	;
COMMAND_ERASE_SECTOR equ 0x7



	; COMMAND_STATUS_OK
	; =================
	;
	; Returned when a command was executed successfully.
	;
COMMAND_STATUS_OK          equ 0x00



bootloader_data udata
jedecid: res 3
bytecounter: res 1
pagecounter: res 1
crc_temp: res 1
crc_low: res 1
crc_high: res 1
xbee_receive_bytes_left: res 1
xbee_receive_address: res 8
xbee_receive_checksum: res 1
xbee_transmit_checksum: res 1
send_length_temp: res 1
	CBUF_DECLARE rxbuf
rcif_fsr0: res 2



crc_table_low_data udata
crc_table_low: res 256

crc_table_high_data udata
crc_table_high: res 256

crc_table_rom code
crc_table:
	dw 0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf
	dw 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7
	dw 0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e
	dw 0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876
	dw 0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd
	dw 0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5
	dw 0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c
	dw 0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974
	dw 0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb
	dw 0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3
	dw 0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a
	dw 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72
	dw 0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9
	dw 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1
	dw 0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738
	dw 0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70
	dw 0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7
	dw 0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff
	dw 0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036
	dw 0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e
	dw 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5
	dw 0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd
	dw 0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134
	dw 0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c
	dw 0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3
	dw 0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb
	dw 0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232
	dw 0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a
	dw 0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1
	dw 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9
	dw 0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330
	dw 0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78



SELECT_CHIP macro
	bcf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	endm



DESELECT_CHIP macro
	bsf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	endm



	code
	; Receive interrupt handler.
rcif_handler:
	; Save FSR0.
	movff FSR0L, rcif_fsr0 + 0
	movff FSR0H, rcif_fsr0 + 1

	; Bank select.
	banksel rxbuf

	; Check for overrun error.
	btfsc RCSTA, OERR
	bra rcif_handler_oerr

	; Check for framing error.
	btfsc RCSTA, FERR
	bra rcif_handler_ferr

	; Receive byte.
	movf RCREG, W
	CBUF_PUT rxbuf, 0

	; Consider if we need to assert RTS or not, based on the fill level of the
	; buffer.
	rcall update_rts

	bra rcif_handler_exit

rcif_handler_oerr:
	; Overrun error. Reset USART.
	bcf RCSTA, CREN
	bsf RCSTA, CREN
	bra rcif_handler_exit

rcif_handler_ferr:
	; Framing error. Discard RCREG to clear.
	movf RCREG, W

rcif_handler_exit:
	; Restore FSR0.
	movff rcif_fsr0 + 0, FSR0L
	movff rcif_fsr0 + 1, FSR0H

	; Done.
	retfie FAST



update_rts:
	; Get buffer fill level.
	CBUF_LEVEL rxbuf
	; If fill level is >= 12, deassert RTS.
	addlw -12
	bc update_rts_deassert
	addlw 12
	; If fill level is <= 10, assert RTS.
	addlw -11
	bnc update_rts_assert
	; Histeresis: leave RTS alone.
	return

update_rts_deassert:
	bsf LAT_RTS, PIN_RTS
	return

update_rts_assert:
	bcf LAT_RTS, PIN_RTS
	return



	; Main code.
bootload:
	; Take control of the SPI bus.
	SPI_DRIVE

	; Allow writes to the Flash chip.
	bcf LAT_FLASH_WP, PIN_FLASH_WP

	; Send the JEDEC ID command (0x9F) and save the response.
	banksel jedecid
	SELECT_CHIP
	SPI_SEND_CONSTANT 0x9F
	SPI_RECEIVE jedecid + 0
	SPI_RECEIVE jedecid + 1
	SPI_RECEIVE jedecid + 2
	DESELECT_CHIP

	; Initialize the circular buffer.
	CBUF_INIT rxbuf

	; Load the CRC table from ROM to RAM.
	; Insanely enough, PLUSW treats WREG as a signed number in the range -128 to
	; +127 (NOT an unsigned number!). So we want to point the FSRs at
	; table_base + 128, so that the full scale of WREG gives the full range of
	; table entries.
	movlw LOW(crc_table)
	movwf TBLPTRL
	movlw HIGH(crc_table)
	movwf TBLPTRH
	movlw UPPER(crc_table)
	movwf TBLPTRU
	lfsr 0, crc_table_low + 128
	lfsr 1, crc_table_high + 128
	movlw 0
load_crc_table_loop:
	tblrd *+
	movff TABLAT, PLUSW0
	tblrd *+
	movff TABLAT, PLUSW1
	addlw 1
	bnz load_crc_table_loop

	; Start up the USART.
	clrf RCSTA
	clrf TXSTA
	clrf SPBRGH
	movlw 7
	movwf SPBRG
	movlw (1 << BRG16)
	movwf BAUDCON
	movlw (1 << TXEN) | (1 << BRGH)
	movwf TXSTA
	movlw (1 << SPEN) | (1 << CREN)
	movwf RCSTA
	nop
	nop
	bsf PIE1, RCIE

expecting_sop:
	; Receive a raw byte from the USART without unescaping. It should be 0x7E.
	rcall receive_raw
	xorlw 0x7E
	bnz expecting_sop

got_sop:
	; Receive a byte. It should be the MSB of the length, which should be zero.
	rcall receive_byte_semicooked
	xorlw 0
	bnz expecting_sop

	; Receive a byte. It should be the LSB of the length, which should be no longer than 100 bytes.
	rcall receive_byte_semicooked
	movwf xbee_receive_bytes_left
	movlw 101
	cpfslt xbee_receive_bytes_left
	bra expecting_sop

	; Initialize the receive checksum.
	clrf xbee_receive_checksum

	; Receive the API ID, which should be 0x80 (64-bit receive).
	rcall receive_byte_cooked
	xorlw 0x80
	bnz expecting_sop

	; Receive the remote peer's address.
	rcall receive_byte_cooked
	movwf xbee_receive_address + 0
	rcall receive_byte_cooked
	movwf xbee_receive_address + 1
	rcall receive_byte_cooked
	movwf xbee_receive_address + 2
	rcall receive_byte_cooked
	movwf xbee_receive_address + 3
	rcall receive_byte_cooked
	movwf xbee_receive_address + 4
	rcall receive_byte_cooked
	movwf xbee_receive_address + 5
	rcall receive_byte_cooked
	movwf xbee_receive_address + 6
	rcall receive_byte_cooked
	movwf xbee_receive_address + 7

	; Receive the RSSI and discard it.
	rcall receive_byte_cooked

	; Receive the OPTIONS byte and discard it.
	rcall receive_byte_cooked

	; Receive the COMMAND ID and dispatch based on it.
	rcall receive_byte_cooked
	DISPATCH_INIT
	DISPATCH_COND COMMAND_IDENT, handle_ident
	DISPATCH_COND COMMAND_ERASE_BLOCK, handle_erase_block
	DISPATCH_BRA  COMMAND_WRITE_PAGE1, handle_write_page1
	DISPATCH_BRA  COMMAND_CRC_SECTOR, handle_crc_sector
	DISPATCH_GOTO COMMAND_ERASE_SECTOR, handle_erase_sector
	DISPATCH_END_RESTORE

	; COMMAND ID is illegal.
	goto handle_error



handle_ident:
	; Page number is ignored.
	rcall receive_byte_cooked
	rcall receive_byte_cooked

	; Should be no payload, so checksum should be next.
	rcall receive_and_check_checksum

	; Send back the IDENT response.
	rcall send_sop
	movlw 21
	rcall send_length
	movlw 0x00
	rcall send_byte
	movlw 0x00
	rcall send_byte
	rcall send_address
	movlw 0x00
	rcall send_byte
	movlw COMMAND_IDENT
	rcall send_byte
	movlw COMMAND_STATUS_OK
	rcall send_byte
	movlw 'T'
	rcall send_byte
	movlw 'B'
	rcall send_byte
	movlw 'O'
	rcall send_byte
	movlw 'T'
	rcall send_byte
	movlw 'S'
	rcall send_byte
	movf jedecid + 0, W
	rcall send_byte
	movf jedecid + 1, W
	rcall send_byte
	movf jedecid + 2, W
	rcall send_byte
	rcall send_checksum

	; Continue.
	bra expecting_sop



handle_erase_block:
	; Send the WRITE ENABLE command.
	rcall send_write_enable

	; Start sending the BLOCK ERASE command (0xD8).
	SELECT_CHIP
	SPI_SEND_CONSTANT 0xD8

	; Receive and pass on the page number.
	rcall receive_byte_cooked
	SPI_SEND_WREG
	rcall receive_byte_cooked
	SPI_SEND_WREG
	SPI_SEND_CONSTANT 0
	DESELECT_CHIP

	; Expect the checksum next.
	rcall receive_and_check_checksum

	; Wait until operation completes.
	rcall wait_busy

	; Done.
	bra expecting_sop



wait_busy:
	; Poll the STATUS byte in the Flash chip until the chip is no longer busy.
	SELECT_CHIP
	SPI_SEND_CONSTANT 0x05
wait_busy_loop:
	SPI_RECEIVE WREG
	btfsc WREG, 0
	bra wait_busy_loop
	DESELECT_CHIP
	return



send_write_enable:
	SELECT_CHIP
	SPI_SEND_CONSTANT 0x06
	DESELECT_CHIP
	return



send_sop:
	; Transmit the raw byte 0x7E.
	movlw 0x7E
	bra send_raw



send_length:
	; The length is two bytes long, the first of which is zero. The length is not included in the checksum.
	movwf send_length_temp
	movlw 0
	rcall send_byte
	movf send_length_temp, W
	rcall send_byte

	; Clear checksum because we're starting a new packet.
	setf xbee_transmit_checksum

	; Done.
	return



send_address:
	movf xbee_receive_address + 0, W
	rcall send_byte
	movf xbee_receive_address + 1, W
	rcall send_byte
	movf xbee_receive_address + 2, W
	rcall send_byte
	movf xbee_receive_address + 3, W
	rcall send_byte
	movf xbee_receive_address + 4, W
	rcall send_byte
	movf xbee_receive_address + 5, W
	rcall send_byte
	movf xbee_receive_address + 6, W
	rcall send_byte
	movf xbee_receive_address + 7, W
	bra send_byte



send_checksum:
	; Send the checksum calculated so far.
	movf xbee_transmit_checksum, W
	bra send_byte



send_byte:
	; Update the checksum.
	subwf xbee_transmit_checksum, F

	; Some bytes need escaping.
	DISPATCH_INIT
	DISPATCH_COND 0x7E, send_byte_7e
	DISPATCH_COND 0x7D, send_byte_7d
	DISPATCH_COND 0x11, send_byte_11
	DISPATCH_COND 0x13, send_byte_13
	DISPATCH_END_RESTORE

	; This byte does not need escaping. Send it.
	bra send_raw

send_byte_7e:
	movlw 0x7D
	rcall send_raw
	movlw 0x7E ^ 0x20
	bra send_raw

send_byte_7d:
	movlw 0x7D
	rcall send_raw
	movlw 0x7D ^ 0x20
	bra send_raw

send_byte_11:
	movlw 0x7D
	rcall send_raw
	movlw 0x11 ^ 0x20
	bra send_raw

send_byte_13:
	movlw 0x7D
	rcall send_raw
	movlw 0x13 ^ 0x20
	bra send_raw



send_raw:
	; Wait until the transmit buffer is not full.
	btfss PIR1, TXIF
	bra send_raw

	; Send the byte.
	movwf TXREG
	return



receive_and_check_checksum:
	; Check that we've used up all the data.
	tstfsz xbee_receive_bytes_left
	bra handle_error

	; Receive the checksum byte and add it onto the accumulated value.
	rcall receive_byte_semicooked
	addwf xbee_receive_checksum, W

	; For a correct packet, the checksum should be 0xFF.
	xorlw 0xFF
	bnz handle_error
	return



receive_byte_cooked:
	; Check that there are bytes left.
	movf xbee_receive_bytes_left, F
	bz handle_error

	; Use one of them up.
	decf xbee_receive_bytes_left, F

	; Receive the byte and add it to the checksum.
	rcall receive_byte_semicooked
	addwf xbee_receive_checksum, F
	return



receive_byte_semicooked:
	; Receive a byte from the USART.
	rcall receive_raw

	; Check if this is a special byte.
	DISPATCH_INIT
	DISPATCH_COND 0x7E, handle_unexpected_sop
	DISPATCH_COND 0x7D, receive_byte_semicooked_escaped
	DISPATCH_END_RESTORE

	; Nothing special. Return it.
	return

receive_byte_semicooked_escaped:
	; We need to receive another byte and unescape it.
	rcall receive_raw
	DISPATCH_INIT
	DISPATCH_COND 0x7E, handle_unexpected_sop
	DISPATCH_END_RESTORE
	xorlw 0x20
	return



receive_raw:
	; Check if the bootload pin has been deasserted (gone low).
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	reset

	; Check if the emergency erase pin has been asserted (gone low).
	btfss PORT_EMERG_ERASE, PIN_EMERG_ERASE
	reset

	; Check if there's data available to receive.
	; HACK: once mode pin checking goes to interrupts, replace all this with a
	; simple call to CBUF_GET!
	CBUF_LEVEL rxbuf
	addlw 0
	bz receive_raw

	; Consider if we need to assert RTS or not, based on the fill level of the
	; buffer.
	rcall update_rts

	; Get the byte.
	CBUF_GET rxbuf, 0

	; Done.
	return



handle_error:
	; We might have detected the error at any level of call. Clear the stack.
	clrf STKPTR

	; We might be in the middle of sending data to the Flash. Deselect it.
	DESELECT_CHIP

	; The Flash might be in the middle of doing an erase or write. Wait for it.
	rcall wait_busy

	; Keep pumping packets.
	bra expecting_sop



handle_unexpected_sop:
	; We might have detected the error at any level of call. Clear the stack.
	clrf STKPTR

	; We might be in the middle of sending data to the Flash. Deselect it.
	DESELECT_CHIP

	; The Flash might be in the middle of doing an erase or write. Wait for it.
	rcall wait_busy

	; Keep pumping packets.
	bra got_sop



handle_write_page1:
	; Send the WRITE ENABLE command.
	rcall send_write_enable
	
	; Start sending the PAGE PROGRAM command (0x02).
	SELECT_CHIP
	SPI_SEND_CONSTANT 0x02

	; Receive and pass the page number.
	rcall receive_byte_cooked
	SPI_SEND_WREG
	rcall receive_byte_cooked
	SPI_SEND_WREG
	SPI_SEND_CONSTANT 0

	; Receive and pass the data.
	movlw 86
	movwf bytecounter
handle_write_page1_loop:
	rcall receive_byte_cooked
	SPI_SEND_WREG
	decfsz bytecounter, F
	bra handle_write_page1_loop

	; Expect the checksum next.
	rcall receive_and_check_checksum

	; It's now time to receive the WRITE PAGE 2 packet.
	; Receive a raw byte from the USART without unescaping. It should be 0x7E.
	rcall receive_raw
	xorlw 0x7E
	skpz
	bra expecting_sop

	; Receive a byte. It should be the MSB of the length, which should be zero.
	rcall receive_byte_semicooked
	xorlw 0
	skpz
	bra expecting_sop

	; Receive a byte. It should be the LSB of the length, which should be no longer than 100 bytes.
	rcall receive_byte_semicooked
	movwf xbee_receive_bytes_left
	movlw 101
	cpfslt xbee_receive_bytes_left
	bra expecting_sop

	; Initialize the receive checksum.
	clrf xbee_receive_checksum

	; Receive the API ID, which should be 0x80 (64-bit receive).
	rcall receive_byte_cooked
	xorlw 0x80
	skpz
	bra expecting_sop

	; Receive the remote peer's address.
	rcall receive_byte_cooked
	movwf xbee_receive_address + 0
	rcall receive_byte_cooked
	movwf xbee_receive_address + 1
	rcall receive_byte_cooked
	movwf xbee_receive_address + 2
	rcall receive_byte_cooked
	movwf xbee_receive_address + 3
	rcall receive_byte_cooked
	movwf xbee_receive_address + 4
	rcall receive_byte_cooked
	movwf xbee_receive_address + 5
	rcall receive_byte_cooked
	movwf xbee_receive_address + 6
	rcall receive_byte_cooked
	movwf xbee_receive_address + 7

	; Receive the RSSI and discard it.
	rcall receive_byte_cooked

	; Receive the OPTIONS byte and discard it.
	rcall receive_byte_cooked

	; Receive the COMMAND ID, which should be WRITE PAGE 2.
	rcall receive_byte_cooked
	xorlw COMMAND_WRITE_PAGE2
	skpz
	bra handle_error

	; Ignore the page number.
	rcall receive_byte_cooked
	rcall receive_byte_cooked

	; Receive and pass the data.
	movlw 86
	movwf bytecounter
handle_write_page2_loop:
	rcall receive_byte_cooked
	SPI_SEND_WREG
	decfsz bytecounter, F
	bra handle_write_page2_loop

	; Expect the checksum next.
	rcall receive_and_check_checksum

	; It's now time to receive the WRITE PAGE 3 packet.
	; Receive a raw byte from the USART without unescaping. It should be 0x7E.
	rcall receive_raw
	xorlw 0x7E
	skpz
	bra expecting_sop

	; Receive a byte. It should be the MSB of the length, which should be zero.
	rcall receive_byte_semicooked
	xorlw 0
	skpz
	bra expecting_sop

	; Receive a byte. It should be the LSB of the length, which should be no longer than 100 bytes.
	rcall receive_byte_semicooked
	movwf xbee_receive_bytes_left
	movlw 101
	cpfslt xbee_receive_bytes_left
	bra expecting_sop

	; Initialize the receive checksum.
	clrf xbee_receive_checksum

	; Receive the API ID, which should be 0x80 (64-bit receive).
	rcall receive_byte_cooked
	xorlw 0x80
	skpz
	bra expecting_sop

	; Receive the remote peer's address.
	rcall receive_byte_cooked
	movwf xbee_receive_address + 0
	rcall receive_byte_cooked
	movwf xbee_receive_address + 1
	rcall receive_byte_cooked
	movwf xbee_receive_address + 2
	rcall receive_byte_cooked
	movwf xbee_receive_address + 3
	rcall receive_byte_cooked
	movwf xbee_receive_address + 4
	rcall receive_byte_cooked
	movwf xbee_receive_address + 5
	rcall receive_byte_cooked
	movwf xbee_receive_address + 6
	rcall receive_byte_cooked
	movwf xbee_receive_address + 7

	; Receive the RSSI and discard it.
	rcall receive_byte_cooked

	; Receive the OPTIONS byte and discard it.
	rcall receive_byte_cooked

	; Receive the COMMAND ID, which should be WRITE PAGE 3.
	rcall receive_byte_cooked
	xorlw COMMAND_WRITE_PAGE3
	skpz
	bra handle_error

	; Ignore the page number.
	rcall receive_byte_cooked
	rcall receive_byte_cooked

	; Receive and pass the data.
	movlw 84
	movwf bytecounter
handle_write_page3_loop:
	rcall receive_byte_cooked
	SPI_SEND_WREG
	decfsz bytecounter, F
	bra handle_write_page3_loop

	; Finish SPI command.
	DESELECT_CHIP

	; Expect the checksum next.
	rcall receive_and_check_checksum

	; Wait until operation completes.
	rcall wait_busy

	; Done.
	bra expecting_sop



handle_crc_sector:
	; Start sending the READ DATA command (0x03).
	SELECT_CHIP
	SPI_SEND_CONSTANT 0x03

	; Receive and pass on the page number.
	rcall receive_byte_cooked
	SPI_SEND_WREG
	rcall receive_byte_cooked
	SPI_SEND_WREG
	SPI_SEND_CONSTANT 0

	; Expect the checksum next.
	rcall receive_and_check_checksum

	; Start the response. We'll stream the CRCs to the XBee.
	rcall send_sop
	movlw 45
	rcall send_length
	movlw 0x00
	rcall send_byte
	movlw 0x00
	rcall send_byte
	rcall send_address
	movlw 0x00
	rcall send_byte
	movlw COMMAND_CRC_SECTOR
	rcall send_byte
	movlw COMMAND_STATUS_OK
	rcall send_byte

	; Set up a counter of pages.
	banksel pagecounter
	movlw 16
	movwf pagecounter
	clrf bytecounter

	; Set FSR0 to point at low bytes and FSR1 at high bytes of CRC table.
	; See entry point for explanation of why the + 128.
	lfsr 0, crc_table_low + 128
	lfsr 1, crc_table_high + 128

	; Go into a loop of pages.
handle_crc_sector_pageloop:
	; Initialize the CRC to 0xFFFF.
	setf crc_low
	setf crc_high

	; Go into a loop of bytes.
handle_crc_sector_byteloop:
	; Receive one byte from the SPI port into WREG.
	SPI_RECEIVE WREG

	; Perform the CRC update.
	xorwf crc_low, W
	movff crc_high, crc_low
	movff PLUSW1, crc_high
	movf PLUSW0, W
	xorwf crc_low, F

	; Decrement byte count and loop if nonzero.
	decfsz bytecounter, F
	bra handle_crc_sector_byteloop

	; A page is finished. Stream out the page's CRC.
	movf crc_low, W
	rcall send_byte
	movf crc_high, W
	rcall send_byte

	; Decrement page count and loop if nonzero.
	decfsz pagecounter, F
	bra handle_crc_sector_pageloop

	; Deselect the chip.
	DESELECT_CHIP

	; Finish response.
	rcall send_checksum

	; Done.
	goto expecting_sop



handle_erase_sector:
	; Send the WRITE ENABLE command.
	rcall send_write_enable

	; Start sending the SECTOR ERASE command (0x20).
	SELECT_CHIP
	SPI_SEND_CONSTANT 0x20

	; Receive and pass on the page number.
	rcall receive_byte_cooked
	SPI_SEND_WREG
	rcall receive_byte_cooked
	SPI_SEND_WREG
	SPI_SEND_CONSTANT 0
	DESELECT_CHIP

	; Expect the checksum next.
	rcall receive_and_check_checksum

	; Wait until operation completes.
	rcall wait_busy

	; Done.
	goto expecting_sop

	end
