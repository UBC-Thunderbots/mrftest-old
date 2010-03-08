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
#include "led.inc"
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

	; COMMAND_FPGA_WRITE_DATA
	; =======================
	;
	; Begins a write operation.
	;
	; Page number:
	;  The page number of the page to write
	;
	; Request data:
	;  RLE-compressed data to burn
	;
	; Response data:
	;  None
	;
COMMAND_FPGA_WRITE_DATA equ 0x2

	; COMMAND_FPGA_CRC_CHUNK
	; ======================
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
COMMAND_FPGA_CRC_CHUNK equ 0x3

	; COMMAND_FPGA_ERASE_SECTOR
	; =========================
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
COMMAND_FPGA_ERASE_SECTOR equ 0x4

	; COMMAND_PIC_READ_FUSES
	; ======================
	;
	; Reads the configuration fuse area on the PIC.
	;
	; Page number:
	;  Ignored
	;
	; Request data:
	;  None
	;
	; Response data:
	;  16 bytes of configuration fuse data, from 0x300000 to 0x30000F.
	;
COMMAND_PIC_READ_FUSES equ 0x5

	; COMMAND_PIC_WRITE_DATA
	; ======================
	;
	; Writes a chunk of data to the PIC's Flash memory.
	;
	; Page number:
	;  The address at which to start writing, which must be a multiple of 64 and
	;  which, for reliable operation, should probably point somewhere in the
	;  staging area (0x4800 through 0x7FFF).
	;
	; Request data:
	;  64 bytes of data to burn
	;
	; Response data:
	;  64 bytes read back from the target area after the burn finished
	;
COMMAND_PIC_WRITE_DATA equ 0x6

	; COMMAND_PIC_ENABLE_UPGRADE
	; ==========================
	;
	; Sets the internal flag that signals the boot block to copy the staging
	; area to the execution area on the next reboot.
	;
	; Page number:
	;  Ignored
	;
	; Request data:
	;  None
	;
	; Response data:
	;  The final value of the internal flag, which when all is OK should be
	;  0x1234.
	;
COMMAND_PIC_ENABLE_UPGRADE equ 0x7



	; COMMAND_STATUS_OK
	; =================
	;
	; Returned when a command was executed successfully.
	;
COMMAND_STATUS_OK          equ 0x00



	; CRC_INIT_FSRS
	; =============
	;
	; Initializes FSR0 and FSR1 suitable to call CRC_UPDATE_WREG.
	;
CRC_INIT_FSRS macro
	lfsr 0, crc_table_low + 128
	lfsr 1, crc_table_high + 128
	endm

	; CRC_UPDATE_WREG
	; ===============
	;
	; Updates the CRC stored in crc_high:crc_low with the byte stored in WREG,
	; assuming FSR0 points 128 bytes into crc_table_low and FSR1 points 128
	; bytes into crc_table_high. WREG is destroyed.
	;
CRC_UPDATE_WREG macro
	xorwf crc_low, W
	movff crc_high, crc_low
	movff PLUSW1, crc_high
	movf PLUSW0, W
	xorwf crc_low, F
	endm



bootloader_data udata
jedecid: res 3
bytecounter: res 1
pagecounter: res 1
crc_low: res 1
crc_high: res 1
xbee_receive_bytes_left: res 1
xbee_receive_address: res 8
xbee_receive_checksum: res 1
xbee_transmit_checksum: res 1
send_length_temp: res 1
	CBUF_DECLARE rxbuf
rcif_fsr0: res 2
page_number: res 2
old_page_number: res 2
page_bitmap: res 2
bits_table: res 8
write_repeat_temp: res 1
pic_write_buffer: res 64



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
	; Enable mode-change interrupts.
	; We care about both signals (EMERG_ERASE=INT2, BOOTLOAD=INT1).
	; EMERG_ERASE should be high right now (deasserted).
	; BOOTLOAD should be high right now (asserted).
	; A change on either one should reset the PIC.
	bcf INTCON2, INTEDG1
	bcf INTCON2, INTEDG2
	bcf INTCON3, INT1IF
	bcf INTCON3, INT2IF
	bsf INTCON3, INT1IE
	bsf INTCON3, INT2IE

	; Check that we haven't raced and missed a change.
	btfss PORT_EMERG_ERASE, PIN_EMERG_ERASE
	reset
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	reset

	; Take control of the SPI bus.
	SPI_DRIVE

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
	CRC_INIT_FSRS
	movlw 0
load_crc_table_loop:
	tblrd *+
	movff TABLAT, PLUSW0
	tblrd *+
	movff TABLAT, PLUSW1
	addlw 1
	bnz load_crc_table_loop

	; Initialize the table of bits by position.
	movlw 0x01
	movwf bits_table + 0
	movlw 0x02
	movwf bits_table + 1
	movlw 0x04
	movwf bits_table + 2
	movlw 0x08
	movwf bits_table + 3
	movlw 0x10
	movwf bits_table + 4
	movlw 0x20
	movwf bits_table + 5
	movlw 0x40
	movwf bits_table + 6
	movlw 0x80
	movwf bits_table + 7

	; Start up the USART.
	clrf RCSTA
	clrf TXSTA
	clrf SPBRGH
	movlw 1
	movwf SPBRG
	movlw (1 << BRG16)
	movwf BAUDCON
	movlw (1 << TXEN)
	movwf TXSTA
	movlw (1 << SPEN) | (1 << CREN)
	movwf RCSTA
	nop
	nop
	bsf PIE1, RCIE

expecting_sop:
	; We are currently idle.
	call led_idle

expecting_sop_loop:
	; Receive a raw byte from the USART without unescaping. It should be 0x7E.
	rcall receive_raw
	xorlw 0x7E
	bnz expecting_sop_loop

got_sop:
	; We got a SOP. We consider this to be packet activity.
	call led_activity

	; Receive a byte. It should be the MSB of the length, which should be zero.
	rcall receive_byte_semicooked
	xorlw 0
	bnz expecting_sop

	; Receive a byte. It should be the LSB of the length, which should be no longer than 111 bytes.
	rcall receive_byte_semicooked
	movwf xbee_receive_bytes_left
	movlw 112
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
main_dispatch_tree:
	DISPATCH_INIT
	DISPATCH_COND COMMAND_IDENT, handle_ident
	DISPATCH_BRA  COMMAND_FPGA_WRITE_DATA, handle_fpga_write_data
	DISPATCH_BRA  COMMAND_FPGA_CRC_CHUNK, handle_fpga_crc_chunk
	DISPATCH_GOTO COMMAND_FPGA_ERASE_SECTOR, handle_fpga_erase_sector
	DISPATCH_GOTO COMMAND_PIC_READ_FUSES, handle_pic_read_fuses
	DISPATCH_GOTO COMMAND_PIC_WRITE_DATA, handle_pic_write_data
	DISPATCH_GOTO COMMAND_PIC_ENABLE_UPGRADE, handle_pic_enable_upgrade
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
	; Consider if we need to assert RTS or not, based on the fill level of the
	; buffer.
	rcall update_rts

	; Get the byte.
	CBUF_GET rxbuf, 2

	; Done.
	return



handle_error:
	; We might have detected the error at any level of call. Clear the stack.
	clrf STKPTR

	; If we were in the middle of a Page Program command or other write command
	; when the failure occurred, we might be able to abort the operation by
	; sending four extra bits, which will make the transmitted sequence not be a
	; multiple of eight bits in length. So, pulse the SPI Clock line four times.
	; If the chip was already deselected, this will do absolutely nothing.
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK

	; We might be in the middle of sending data to the Flash. Deselect it.
	DESELECT_CHIP

	; The Flash might be in the middle of doing an erase or write. Wait for it.
	rcall wait_busy

	; Keep pumping packets.
	bra expecting_sop



handle_unexpected_sop:
	; We might have detected the error at any level of call. Clear the stack.
	clrf STKPTR

	; If we were in the middle of a Page Program command or other write command
	; when the failure occurred, we might be able to abort the operation by
	; sending four extra bits, which will make the transmitted sequence not be a
	; multiple of eight bits in length. So, pulse the SPI Clock line four times.
	; If the chip was already deselected, this will do absolutely nothing.
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK

	; We might be in the middle of sending data to the Flash. Deselect it.
	DESELECT_CHIP

	; The Flash might be in the middle of doing an erase or write. Wait for it.
	rcall wait_busy

	; Keep pumping packets.
	bra got_sop



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



handle_fpga_write_data:
	call led_off
	; Send the WRITE ENABLE command.
	rcall send_write_enable
	
	; Start sending the PAGE PROGRAM command (0x02).
	SELECT_CHIP
	SPI_SEND_CONSTANT 0x02

	; Receive and pass the page number.
	rcall receive_byte_cooked
	SPI_SEND_WREG
	movwf page_number + 0
	rcall receive_byte_cooked
	SPI_SEND_WREG
	movwf page_number + 1
	SPI_SEND_CONSTANT 0

	; Check if the chunk number has changed.
	movf page_number + 0, W
	xorwf old_page_number + 0, F
	movf page_number + 1, W
	xorwf old_page_number + 1, W
	andlw 0xF0
	iorwf old_page_number + 0, W
	bz handle_fpga_write_data_no_reset_page_bitmap

	; Chunk number changed. Reset written-pages bitmap.
	clrf page_bitmap + 0
	clrf page_bitmap + 1

handle_fpga_write_data_no_reset_page_bitmap:
	; Copy the current page number to the old page number.
	movff page_number + 0, old_page_number + 0
	movff page_number + 1, old_page_number + 1

	; Initialize the CRC to 0xFFFF.
	setf crc_high
	setf crc_low

	; Prepare the FSRs to do CRC calculations.
	CRC_INIT_FSRS

handle_fpga_write_data_payload:
	; Check if there's any more data left.
	movf xbee_receive_bytes_left, F
	skpnz
	bra handle_fpga_write_data_eop

	; Receive the command code.
	rcall receive_byte_cooked

	; If bit 7 is set, it's a repeat code.
	btfsc WREG, 7
	bra handle_fpga_write_data_payload_repeat

	; If the byte is 0x00, it's a termination marker.
	addlw 0
	bz handle_fpga_write_data_payload_term

	; Otherwise, it's literal data. Receive, pass, and CRC.
	movwf bytecounter
handle_fpga_write_data_payload_literal_loop:
	rcall receive_byte_cooked
	SPI_SEND_WREG
	CRC_UPDATE_WREG
	decfsz bytecounter, F
	bra handle_fpga_write_data_payload_literal_loop
	bra handle_fpga_write_data_payload

	; Handle repeat command.
handle_fpga_write_data_payload_repeat:
	; Lower 7 bits are the number of times to repeat.
	andlw 0x7F
	movwf bytecounter
	; Receive the byte to repeat.
	rcall receive_byte_cooked
	movwf write_repeat_temp
	; Send it and update CRC repeatedly.
handle_fpga_write_data_payload_repeat_loop:
	movf write_repeat_temp, W
	SPI_SEND_WREG
	CRC_UPDATE_WREG
	decfsz bytecounter, F
	bra handle_fpga_write_data_payload_repeat_loop
	bra handle_fpga_write_data_payload

	; Handle a termination marker.
handle_fpga_write_data_payload_term:
	; Receive and check the CRC.
	rcall receive_byte_cooked
	xorwf crc_high, W
	skpz
	bra handle_error
	rcall receive_byte_cooked
	xorwf crc_low, W
	skpz
	bra handle_error

	; The next thing should be the packet checksum.
	rcall receive_and_check_checksum

	; Finish the write operation.
	DESELECT_CHIP
	rcall wait_busy

	; We have now written a page. Mark it in the bitmap.
	lfsr 0, page_bitmap
	lfsr 1, bits_table
	movf page_number + 1, W
	andlw 0x0F
	btfsc WREG, 3
	incf FSR0L, F
	andlw 0x07
	movf PLUSW1, W
	iorwf INDF0, F

	; Done.
	bra expecting_sop

	; Handle end of radio packet without termination marker.
handle_fpga_write_data_eop:
	; This is perfectly legal; it just means that we ran out of space and need
	; to use more than one radio packet to carry a full page of data. What we
	; want to do is leave the SPI slave select asserted and the write operation
	; pending, and then receive another packet which will hopefully contain more
	; data for the same page.
	; The first thing we should see is the SOP for the next packet.
	rcall receive_raw
	xorlw 0x7E
	bnz handle_fpga_write_data_eop

	; Receive a byte. It should be the MSB of the length, which should be zero.
	rcall receive_byte_semicooked
	xorlw 0
	bnz handle_fpga_write_data_eop

	; Receive a byte. It should be the LSB of the length, which should be no longer than 111 bytes.
	rcall receive_byte_semicooked
	movwf xbee_receive_bytes_left
	movlw 112
	cpfslt xbee_receive_bytes_left
	bra handle_fpga_write_data_eop

	; Initialize the receive checksum.
	clrf xbee_receive_checksum

	; Receive the API ID, which should be 0x80 (64-bit receive).
	rcall receive_byte_cooked
	xorlw 0x80
	bnz handle_fpga_write_data_eop

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

	; Receive the COMMAND ID.
	rcall receive_byte_cooked

	; If the upper bit is set, this is run data, not bootload data. Ignore it.
	btfsc WREG, 7
	bra handle_fpga_write_data_eop

	; It should be another WRITE DATA. Check.
	DISPATCH_INIT
	DISPATCH_COND COMMAND_FPGA_WRITE_DATA, handle_fpga_write_data_eop_compare_page
	DISPATCH_END_RESTORE

	; It's not. Abort the SPI operation and go back to the main dispatcher.
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	bsf LAT_SPI_CK, PIN_SPI_CK
	bcf LAT_SPI_CK, PIN_SPI_CK
	DESELECT_CHIP
	bra main_dispatch_tree

handle_fpga_write_data_eop_compare_page:
	; This is a second (or subsequent) WRITE DATA command. We need to check that
	; the page number is the same as it was for the original command.
	rcall receive_byte_cooked
	xorwf page_number + 0, W
	skpz
	bra handle_error
	rcall receive_byte_cooked
	xorwf page_number + 1, W
	skpz
	bra handle_error

	; Go back to the main WRITE DATA loop where we grab command bytes, do RLE
	; decoding, and push data through the CRC and the SPI bus.
	bra handle_fpga_write_data_payload



handle_fpga_crc_chunk:
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

	; Start the response. We'll stream the page bitmap and the CRCs to the XBee.
	rcall send_sop
	movlw 47
	rcall send_length
	movlw 0x00
	rcall send_byte
	movlw 0x00
	rcall send_byte
	rcall send_address
	movlw 0x00
	rcall send_byte
	movlw COMMAND_FPGA_CRC_CHUNK
	rcall send_byte
	movlw COMMAND_STATUS_OK
	rcall send_byte

	; Send the page bitmap.
	movf page_bitmap + 1, W
	rcall send_byte
	movf page_bitmap + 0, W
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
handle_fpga_crc_chunk_pageloop:
	; Initialize the CRC to 0xFFFF.
	setf crc_low
	setf crc_high

	; Go into a loop of bytes.
handle_fpga_crc_chunk_byteloop:
	; Receive one byte from the SPI port into WREG.
	SPI_RECEIVE WREG

	; Perform the CRC update.
	CRC_UPDATE_WREG

	; Decrement byte count and loop if nonzero.
	decfsz bytecounter, F
	bra handle_fpga_crc_chunk_byteloop

	; A page is finished. Stream out the page's CRC.
	movf crc_high, W
	rcall send_byte
	movf crc_low, W
	rcall send_byte

	; Decrement page count and loop if nonzero.
	decfsz pagecounter, F
	bra handle_fpga_crc_chunk_pageloop

	; Deselect the chip.
	DESELECT_CHIP

	; Finish response.
	rcall send_checksum

	; Done.
	goto expecting_sop



handle_fpga_erase_sector:
	; Send the WRITE ENABLE command.
	rcall send_write_enable

	; Start sending the SECTOR ERASE command (0xD8).
	SELECT_CHIP
	SPI_SEND_CONSTANT 0xD8

	; Receive and pass on the page number.
	rcall receive_byte_cooked
	SPI_SEND_WREG
	rcall receive_byte_cooked
	SPI_SEND_WREG
	SPI_SEND_CONSTANT 0
	DESELECT_CHIP

	; Clear the page bitmap.
	clrf page_bitmap + 0
	clrf page_bitmap + 1

	; Expect the checksum next.
	rcall receive_and_check_checksum

	; Wait until operation completes.
	rcall wait_busy

	; Done.
	goto expecting_sop



handle_pic_read_fuses:
	; Page number is ignored.
	rcall receive_byte_cooked
	rcall receive_byte_cooked

	; Should be no payload, so checksum should be next.
	rcall receive_and_check_checksum

	; Start the response.
	rcall send_sop
	movlw 30
	rcall send_length
	movlw 0x00
	rcall send_byte
	movlw 0x00
	rcall send_byte
	rcall send_address
	movlw 0x00
	rcall send_byte
	movlw COMMAND_PIC_READ_FUSES
	rcall send_byte
	movlw COMMAND_STATUS_OK
	rcall send_byte

	; Initialize the pointer.
	movlw LOW(0x300000)
	movwf TBLPTRL
	movlw HIGH(0x300000)
	movwf TBLPTRH
	movlw UPPER(0x300000)
	movwf TBLPTRU

	; Initialize a byte counter.
	movlw 16
	movwf bytecounter

	; Push the data.
handle_pic_read_fuses_loop:
	tblrd *+
	movf TABLAT, W
	rcall send_byte
	decfsz bytecounter, F
	bra handle_pic_read_fuses_loop

	; Finish the packet.
	rcall send_checksum

	; Continue.
	goto expecting_sop



handle_pic_write_data:
	; Receive page number (actually physical address) into TBLPTR.
	rcall receive_byte_cooked
	movwf page_number + 0
	rcall receive_byte_cooked
	movwf page_number + 1

	; Receive 64 bytes of data to burn into RAM buffer.
	lfsr 0, pic_write_buffer
	movlw 64
	movwf bytecounter
handle_pic_write_data_receive_loop:
	rcall receive_byte_cooked
	movwf POSTINC0
	decfsz bytecounter, F
	bra handle_pic_write_data_receive_loop

	; This should be the end of the packet.
	rcall receive_and_check_checksum

	; Erase the 64-byte block.
	clrf TBLPTRU
	movff page_number + 0, TBLPTRH
	movff page_number + 1, TBLPTRL
	bcf INTCON, GIEL
	bcf INTCON, GIEH
	movlw (1 << EEPGD) | (1 << FREE) | (1 << WREN)
	movwf EECON1
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
	clrf EECON1
	bsf INTCON, GIEH
	bsf INTCON, GIEL

	; Write the first 32-byte block. TBLPTR must be within the block to write
	; when the write is initiated, so we want to use the preincrement table
	; write instead of the postincrement. Thus, TBLPTR must start out pointing
	; one byte below the starting address. TBLPTR right now points exactly at
	; the starting address; the easiest way to shift it down by one byte is with
	; a postdecrement table read, so do that before we start.
	tblrd *-
	movlw 32
	movwf bytecounter
	lfsr 0, pic_write_buffer
handle_pic_write_data_write1_loop:
	movff POSTINC0, TABLAT
	tblwt +*
	decfsz bytecounter, F
	bra handle_pic_write_data_write1_loop
	bcf INTCON, GIEL
	bcf INTCON, GIEH
	movlw (1 << EEPGD) | (1 << WREN)
	movwf EECON1
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
	clrf EECON1
	bsf INTCON, GIEH
	bsf INTCON, GIEL

	; Write the second 32-byte block. TBLPTR is already pointing one byte before
	; the start of this block, because it's pointing at the last byte of the
	; block we just wrote. FSR0 is also still pointing at the right place.
	movlw 32
	movwf bytecounter
handle_pic_write_data_write2_loop:
	movff POSTINC0, TABLAT
	tblwt +*
	decfsz bytecounter, F
	bra handle_pic_write_data_write2_loop
	bcf INTCON, GIEL
	bcf INTCON, GIEH
	movlw (1 << EEPGD) | (1 << WREN)
	movwf EECON1
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
	clrf EECON1
	bsf INTCON, GIEH
	bsf INTCON, GIEL

	; Begin sending a response.
	call send_sop
	movlw 78
	call send_length
	movlw 0x00
	call send_byte
	movlw 0x00
	call send_byte
	call send_address
	movlw 0x00
	call send_byte
	movlw COMMAND_PIC_WRITE_DATA
	call send_byte
	movlw COMMAND_STATUS_OK
	call send_byte

	; Point the table pointer back at the start of the 64-byte block.
	movff page_number + 0, TBLPTRH
	movff page_number + 1, TBLPTRL

	; Send the entire written block.
	movlw 64
	movwf bytecounter
handle_pic_write_data_readback_loop:
	tblrd *+
	movf TABLAT, W
	call send_byte
	decfsz bytecounter, F
	bra handle_pic_write_data_readback_loop

	; Finish the packet.
	call send_checksum

	; Done.
	goto expecting_sop



handle_pic_enable_upgrade:
	; Page number is ignored.
	call receive_byte_cooked
	call receive_byte_cooked

	; Should be no payload, so checksum should be next.
	call receive_and_check_checksum

	; The flag is to write 0x1234 into the first two bytes of EEPROM.
	; Write the first byte.
	clrf EEADR
	movlw 0x12
	movwf EEDATA
	bcf INTCON, GIEL
	bcf INTCON, GIEH
	movlw (1 << WREN)
	movwf EECON1
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
handle_pic_enable_upgrade_flag1_loop:
	btfsc EECON1, WR
	bra handle_pic_enable_upgrade_flag1_loop
	clrf EECON1
	bsf INTCON, GIEH
	bsf INTCON, GIEL

	; Write the second byte.
	movlw 1
	movwf EEADR
	movlw 0x34
	movwf EEDATA
	bcf INTCON, GIEL
	bcf INTCON, GIEH
	movlw (1 << WREN)
	movwf EECON1
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
handle_pic_enable_upgrade_flag2_loop:
	btfsc EECON1, WR
	bra handle_pic_enable_upgrade_flag2_loop
	clrf EECON1
	bsf INTCON, GIEH
	bsf INTCON, GIEL

	; Start the response.
	call send_sop
	movlw 16
	call send_length
	movlw 0x00
	call send_byte
	movlw 0x00
	call send_byte
	call send_address
	movlw 0x00
	call send_byte
	movlw COMMAND_PIC_READ_FUSES
	call send_byte
	movlw COMMAND_STATUS_OK
	call send_byte

	; Read the two bytes from EEPROM.
	clrf EEADR
	bsf EECON1, RD
	movf EEDATA, W
	call send_byte
	incf EEADR, F
	bsf EECON1, RD
	movf EEDATA, W
	call send_byte

	; Finish the packet.
	call send_checksum

	; Continue.
	goto expecting_sop

	end
