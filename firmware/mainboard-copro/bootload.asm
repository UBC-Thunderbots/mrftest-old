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
#include "dispatch.inc"
#include "pins.inc"
#include "spi.inc"



	global bootload



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

	; COMMAND_STATUS_BAD_COMMAND
	; ==========================
	;
	; Returned when an invalid command ID is sent.
	;
COMMAND_STATUS_BAD_COMMAND equ 0x01

	; COMMAND_STATUS_BAD_ADDRESS
	; ==========================
	;
	; Returned when the page number associated with a command was illegal.
	;
COMMAND_STATUS_BAD_ADDRESS equ 0x02

	; COMMAND_STATUS_BAD_LENGTH
	; =========================
	;
	; Returned when a length value associated with a command was too large.
	;
COMMAND_STATUS_BAD_LENGTH equ 0x03



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



SELECT_CHIP macro
	bcf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	endm



DESELECT_CHIP macro
	bsf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	endm



	code
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

	; Start up the USART.
	clrf RCSTA
	clrf TXSTA
	clrf SPBRGH
	movlw 15
	movwf SPBRG
	movlw (1 << BRG16)
	movwf BAUDCON
	movlw (1 << TXEN) | (1 << BRGH)
	movwf TXSTA
	movlw (1 << SPEN) | (1 << CREN)
	movwf RCSTA
	nop
	nop

expecting_sop:
	; Assert RTS to allow the XBee to send data.
	bcf LAT_RTS, PIN_RTS

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

	; Hold off bytes.
	bsf LAT_RTS, PIN_RTS

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

	; Hold off bytes.
	bsf LAT_RTS, PIN_RTS

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
	btfss PIR1, RCIF
	bra receive_raw

	; Check for overrun error.
	btfsc RCSTA, OERR
	bra receive_raw_oerr

	; Check for framing error.
	btfsc RCSTA, FERR
	bra receive_raw_ferr

	; Return the byte.
	movf RCREG, W
	return

receive_raw_oerr:
	; Reset the USART.
	bcf RCSTA, CREN
	bsf RCSTA, CREN
	bra handle_error

receive_raw_ferr:
	; Drop the byte.
	movf RCREG, W
	bra handle_error



handle_error:
	; Hold off bytes.
	bsf LAT_RTS, PIN_RTS

	; We might have detected the error at any level of call. Clear the stack.
	clrf STKPTR

	; We might be in the middle of sending data to the Flash. Deselect it.
	DESELECT_CHIP

	; The Flash might be in the middle of doing an erase or write. Wait for it.
	rcall wait_busy

	; Keep pumping packets.
	bra expecting_sop



handle_unexpected_sop:
	; Hold off bytes.
	bsf LAT_RTS, PIN_RTS

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

	; All page data has been received and passed to the Flash.
	; Hold off bytes.
	bsf LAT_RTS, PIN_RTS

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

	; Hold off bytes.
	bsf LAT_RTS, PIN_RTS

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

	; Go into a loop of pages.
handle_crc_sector_pageloop:
	; Initialize the CRC to 0xFFFF.
	setf crc_low
	setf crc_high

	; Go into a loop of bytes.
handle_crc_sector_byteloop:
	; Receive one byte from the SPI port into WREG.
	SPI_RECEIVE WREG

	;
	; Update the CRC16.
	;
	; This is a highly-optimized implementation of this basic algorithm:
	;
	; data ^= crc;
	; data ^= data << 4;
	; crc >>= 8;
	; crc |= data << 8;
	; crc ^= data << 3;
	; crc ^= data >> 4;
	;
	; where "data" is a uint8_t and "crc" is a uint16_t.
	;
	; This algorithm is mathematically equivalent to the Linux kernel's
	; CRC-CCITT algorithm. The equivalence of the C code listed above and the
	; assembly code listed below to Linux's algorithm have both been formally
	; proven by an exhaustive search of the parameter space.
	;
	; Some sources claim this is not the traditional CRC-CCITT algorithm, but
	; is rather the CRC-CCITT algorithm with each input byte bit-reversed and
	; with the final CRC bit-reversed after calculation. This bit-reversal is
	; of course mostly irrelevant to the mathematical properties of the CRC.
	; 
	xorwf crc_low, W
	movwf crc_temp
	movff crc_high, crc_low
	movwf crc_high
	rrncf crc_temp, W
	andlw 0x07
	xorwf crc_high, F
	swapf crc_temp, F
	movf crc_temp, W
	andlw 0xF0
	xorwf crc_high, F
	rrncf crc_temp, W
	andlw 0x07
	xorwf crc_high, F
	swapf crc_temp, W
	xorwf crc_temp, W
	andlw 0x0F
	xorwf crc_low, F
	rrncf crc_temp, W
	andlw 0xF8
	xorwf crc_low, F
	btfsc crc_temp, 4
	btg crc_low, 7

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

	; Hold off bytes.
	bsf LAT_RTS, PIN_RTS

	; Wait until operation completes.
	rcall wait_busy

	; Done.
	goto expecting_sop

	end
