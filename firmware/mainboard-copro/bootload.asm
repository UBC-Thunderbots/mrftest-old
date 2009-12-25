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
#include "led.inc"
#include "pins.inc"
#include "sleep.inc"
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
temp: res 1
buffer: res 32

xbee_receive_buffer:
xbee_receive_length_msb: res 1
xbee_receive_length_lsb: res 1
xbee_receive_apiid: res 1
xbee_receive_address: res 8
xbee_receive_rssi: res 1
xbee_receive_options: res 1
xbee_receive_command: res 1
xbee_receive_page_msb: res 1
xbee_receive_page_lsb: res 1
xbee_receive_data: res 86
xbee_receive_checksum: res 1

xbee_transmit_checksum: res 1

xbee_temp: res 1



	code
	; Main code.
bootload:
	; Take control of the SPI bus.
	call spi_drive

	; Allow writes to the Flash chip.
	bcf LAT_FLASH_WP, PIN_FLASH_WP

	; Send the JEDEC ID command (0x9F) and save the response.
	banksel jedecid
	rcall select_chip
	movlw 0x9F
	call spi_send
	call spi_receive
	movwf jedecid + 0
	call spi_receive
	movwf jedecid + 1
	call spi_receive
	movwf jedecid + 2
	rcall deselect_chip

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

	; In the bootloader, blink LED at 1048ms period with 12.5% duty cycle.
	movlw 6
	call led_blink

main_loop:
	; Receive a packet from the XBee.
	rcall xbee_receive_packet

	; It must be of type 0x80 (64-bit receive).
	movlw 0x80
	cpfseq xbee_receive_apiid
	bra main_loop

	; Dispatch based on command ID.
	movf xbee_receive_command, W
	DISPATCH_INIT
	DISPATCH_COND COMMAND_IDENT, handle_ident
	DISPATCH_COND COMMAND_ERASE_BLOCK, handle_erase_block
	DISPATCH_COND COMMAND_WRITE_PAGE1, handle_write_page1
	DISPATCH_BRA  COMMAND_CRC_SECTOR, handle_crc_sector
	DISPATCH_BRA  COMMAND_ERASE_SECTOR, handle_erase_sector
	DISPATCH_END_RESTORE

	; Illegal command. Send back a response.
	bra error_response_bad_command



handle_ident:
	; Send back the IDENT response.
	rcall xbee_send_sop
	movlw 21
	rcall xbee_send_length
	movlw 0x00
	rcall xbee_send
	movlw 0x00
	rcall xbee_send
	rcall xbee_send_address
	movlw 0x00
	rcall xbee_send
	movlw COMMAND_IDENT
	rcall xbee_send
	movlw COMMAND_STATUS_OK
	rcall xbee_send
	movlw 'T'
	rcall xbee_send
	movlw 'B'
	rcall xbee_send
	movlw 'O'
	rcall xbee_send
	movlw 'T'
	rcall xbee_send
	movlw 'S'
	rcall xbee_send
	movf jedecid + 0, W
	rcall xbee_send
	movf jedecid + 1, W
	rcall xbee_send
	movf jedecid + 2, W
	rcall xbee_send
	rcall xbee_send_checksum
	bra main_loop



handle_erase_block:
	; Check that the page number is within the size of the Flash.
	movlw 33
	cpfslt xbee_receive_page_msb
	bra error_response_bad_address

	; Check that the page number is a multiple of 256.
	tstfsz xbee_receive_page_lsb
	bra error_response_bad_address

	; Send the WRITE ENABLE command.
	rcall send_write_enable

	; Send the BLOCK ERASE command (0xD8).
	rcall select_chip
	movlw 0xD8
	call spi_send
	movf xbee_receive_page_msb, W
	call spi_send
	movlw 0
	call spi_send
	call spi_send
	rcall deselect_chip

	; Poll the busy bit and respond when done.
	bra poll_status_and_respond



handle_write_page1:
	; Check that the page number is within the size of the Flash.
	movlw 33
	cpfslt xbee_receive_page_msb
	bra error_response_bad_address

	; Send the WRITE ENABLE command.
	rcall send_write_enable

	; Send the PAGE PROGRAM command (0x02) with address.
	rcall select_chip
	movlw 0x02
	call spi_send
	movf xbee_receive_page_msb, W
	call spi_send
	movf xbee_receive_page_lsb, W
	call spi_send
	movlw 0
	call spi_send

	; Send the data.
	lfsr 0, xbee_receive_data
	movlw 86
	movwf bytecounter
handle_write_page1_loop:
	movf POSTINC0, W
	call spi_send
	decfsz bytecounter, F
	bra handle_write_page1_loop

handle_write_page1_wait_for_page2:
	; Wait for another packet.
	rcall xbee_receive_packet

	; Check API ID.
	movlw 0x80
	cpfseq xbee_receive_apiid
	bra handle_write_page1_wait_for_page2

	; Check command ID.
	movf xbee_receive_command, W
	xorlw COMMAND_WRITE_PAGE2
	bz handle_write_page2

	; Not WRITE_PAGE2. Deselect chip and send error.
	rcall deselect_chip
	bra error_response_bad_command



handle_write_page2:
	; Send the data.
	lfsr 0, xbee_receive_data
	movlw 86
	movwf bytecounter
handle_write_page2_loop:
	movf POSTINC0, W
	call spi_send
	decfsz bytecounter, F
	bra handle_write_page2_loop

handle_write_page2_wait_for_page3:
	; Wait for another packet.
	rcall xbee_receive_packet

	; Check API ID.
	movlw 0x80
	cpfseq xbee_receive_apiid
	bra handle_write_page2_wait_for_page3

	; Check command ID.
	movf xbee_receive_command, W
	xorlw COMMAND_WRITE_PAGE3
	bz handle_write_page3

	; Not WRITE_PAGE3. Deselect chip and send error.
	rcall deselect_chip
	bra error_response_bad_command



handle_write_page3:
	; Send the data.
	lfsr 0, xbee_receive_data
	movlw 84
	movwf bytecounter
handle_write_page3_loop:
	movf POSTINC0, W
	call spi_send
	decfsz bytecounter, F
	bra handle_write_page3_loop
	rcall deselect_chip

	; Poll the busy bit and continue when done.
	rcall select_chip
	movlw 0x05
	call spi_send
handle_write_page3_busy_loop:
	call spi_receive
	btfsc WREG, 0
	bra handle_write_page3_busy_loop
	rcall deselect_chip
	bra main_loop



handle_crc_sector:
	; Check that the page number is a multiple of 16.
	movf xbee_receive_page_lsb, W
	andlw 0x0F
	bnz error_response_bad_address

	; Check that the page number is within the size of the Flash.
	movlw 33
	cpfslt xbee_receive_page_msb
	bra error_response_bad_address

	; Send the READ DATA command (0x03) with address.
	rcall select_chip
	movlw 0x03
	call spi_send
	movf xbee_receive_page_msb, W
	call spi_send
	movf xbee_receive_page_lsb, W
	call spi_send
	movlw 0
	call spi_send

	; Address the buffer.
	lfsr 2, buffer

	; Set up a counter of pages.
	banksel pagecounter
	movlw 16
	movwf pagecounter
	clrf bytecounter

	; Go into a loop of pages.
handle_crc_sector_pageloop:
	; Initialize the CRC to 0xFFFF.
	setf [0]
	setf [1]

	; Go into a loop of bytes.
handle_crc_sector_byteloop:
	; Receive one byte from the SPI port into WREG.
	call spi_receive

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
	xorwf [0], W
	movwf temp
	movss [1], [0]
	movwf [1]
	rrncf temp, W
	andlw 0x07
	xorwf [1], F
	swapf temp, F
	movf temp, W
	andlw 0xF0
	xorwf [1], F
	rrncf temp, W
	andlw 0x07
	xorwf [1], F
	swapf temp, W
	xorwf temp, W
	andlw 0x0F
	xorwf [0], F
	rrncf temp, W
	andlw 0xF8
	xorwf [0], F
	btfsc temp, 4
	btg [0], 7

	; Decrement byte count and loop if nonzero.
	decfsz bytecounter, F
	bra handle_crc_sector_byteloop

	; A page is finished. Advance the FSR to the next CRC position.
	addfsr 2, 2

	; Decrement page count and loop if nonzero.
	decfsz pagecounter, F
	bra handle_crc_sector_pageloop

	; Deselect the chip.
	rcall deselect_chip

	; Send response.
	rcall xbee_send_sop
	movlw 45
	rcall xbee_send_length
	movlw 0x00
	rcall xbee_send
	movlw 0x00
	rcall xbee_send
	rcall xbee_send_address
	movlw 0x00
	rcall xbee_send
	movlw COMMAND_CRC_SECTOR
	rcall xbee_send
	movlw COMMAND_STATUS_OK
	rcall xbee_send
	movlw 32
	movwf temp
	lfsr 0, buffer
handle_crc_sector_response_data_loop:
	movf POSTINC0, W
	rcall xbee_send
	decfsz temp, F
	bra handle_crc_sector_response_data_loop
	rcall xbee_send_checksum
	bra main_loop



handle_erase_sector:
	; Check that the page number is a multiple of 16.
	movf xbee_receive_page_lsb, W
	andlw 0x0F
	bnz error_response_bad_address

	; Check that the page number is within the size of the Flash.
	movlw 33
	cpfslt xbee_receive_page_msb
	bra error_response_bad_address

	; Send the WRITE ENABLE command.
	rcall send_write_enable

	; Send the SECTOR ERASE command (0x20).
	rcall select_chip
	movlw 0x20
	call spi_send
	movf xbee_receive_page_msb, W
	call spi_send
	movlw 0
	call spi_send
	call spi_send
	rcall deselect_chip

	; Poll the busy bit and respond when done.
	bra poll_status_and_respond



error_response_bad_command:
	movlw COMMAND_STATUS_BAD_COMMAND
	bra error_response_generic



error_response_bad_address:
	movlw COMMAND_STATUS_BAD_ADDRESS
	bra error_response_generic



error_response_bad_length:
	movlw COMMAND_STATUS_BAD_LENGTH
	bra error_response_generic



error_response_generic:
	movwf temp
	rcall xbee_send_sop
	movlw 13
	rcall xbee_send_length
	movlw 0x00
	rcall xbee_send
	movlw 0x00
	rcall xbee_send
	rcall xbee_send_address
	movlw 0x00
	rcall xbee_send
	movf xbee_receive_command, W
	rcall xbee_send
	movf temp, W
	rcall xbee_send
	rcall xbee_send_checksum
	bra main_loop



poll_status_and_respond:
	; Poll the STATUS byte in the Flash chip until the chip is no longer busy.
	rcall select_chip
	movlw 0x05
	call spi_send
poll_status_and_respond_loop:
	call spi_receive
	btfsc WREG, 0
	bra poll_status_and_respond_loop
	rcall deselect_chip

	; Send back an OK.
	rcall xbee_send_sop
	movlw 13
	rcall xbee_send_length
	movlw 0x00
	rcall xbee_send
	movlw 0x00
	rcall xbee_send
	rcall xbee_send_address
	movlw 0x00
	rcall xbee_send
	movf xbee_receive_command, W
	rcall xbee_send
	movlw COMMAND_STATUS_OK
	rcall xbee_send
	rcall xbee_send_checksum
	bra main_loop



select_chip:
	rcall sleep_1us
	bcf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	rcall sleep_1us
	return



deselect_chip:
	rcall sleep_1us
	bsf LAT_SPI_SS_FLASH, PIN_SPI_SS_FLASH
	rcall sleep_1us
	return



send_write_enable:
	rcall select_chip
	movlw 0x06
	call spi_send
	rcall deselect_chip
	return



xbee_send_sop:
	; Clear checksum because we're starting a new packet.
	setf xbee_transmit_checksum

	; Transmit the raw byte 0x7E.
	movlw 0x7E
	bra usart_send



xbee_send_length:
	; The length is two bytes long, the first of which is zero. The length is not included in the checksum.
	movwf xbee_temp
	movlw 0
	rcall xbee_send_without_checksumming
	movf xbee_temp, W
	bra xbee_send_without_checksumming



xbee_send:
	; Normal bytes affect the checksum.
	subwf xbee_transmit_checksum, F
	bra xbee_send_without_checksumming



xbee_send_address:
	; Send the address using the normal algorithm.
	lfsr 1, xbee_receive_address
	movf POSTINC1, W
	rcall xbee_send
	movf POSTINC1, W
	rcall xbee_send
	movf POSTINC1, W
	rcall xbee_send
	movf POSTINC1, W
	rcall xbee_send
	movf POSTINC1, W
	rcall xbee_send
	movf POSTINC1, W
	rcall xbee_send
	movf POSTINC1, W
	rcall xbee_send
	movf POSTINC1, W
	bra xbee_send



xbee_send_checksum:
	; Send the checksum calculated so far.
	movf xbee_transmit_checksum, W
	bra xbee_send_without_checksumming



xbee_send_without_checksumming:
	; Some bytes need escaping.
	DISPATCH_INIT
	DISPATCH_COND 0x7E, xbee_send_without_checksumming_escape_7e
	DISPATCH_COND 0x7D, xbee_send_without_checksumming_escape_7d
	DISPATCH_COND 0x11, xbee_send_without_checksumming_escape_11
	DISPATCH_COND 0x13, xbee_send_without_checksumming_escape_13
	DISPATCH_END_RESTORE

	; This byte does not need escaping. Send it.
	bra usart_send

xbee_send_without_checksumming_escape_7e:
	movlw 0x7D
	rcall usart_send
	movlw 0x7E ^ 0x20
	bra usart_send

xbee_send_without_checksumming_escape_7d:
	movlw 0x7D
	rcall usart_send
	movlw 0x7D ^ 0x20
	bra usart_send

xbee_send_without_checksumming_escape_11:
	movlw 0x7D
	rcall usart_send
	movlw 0x11 ^ 0x20
	bra usart_send

xbee_send_without_checksumming_escape_13:
	movlw 0x7D
	rcall usart_send
	movlw 0x13 ^ 0x20
	bra usart_send



usart_send:
	; Wait until the transmit buffer is not full.
	btfss PIR1, TXIF
	bra usart_send

	; Send the byte.
	movwf TXREG
	return



xbee_receive_packet:
	; Assert RTS to allow the XBee to send data.
	bcf LAT_RTS, PIN_RTS

	; Receive a raw byte from the USART without unescaping. It should be 0x7E.
	; usart_receive participates in the stack hacking, so it expects two levels of function
	; calls between itself and xbee_receive_packet (one for xbee_receive_byte and one for
	; usart_receive). To provide the expected stack layout, push a dummy stack element.
	push
	rcall usart_receive
	pop
	xorlw 0x7E
	bnz xbee_receive_packet

	; Receive a byte. It should be the MSB of the length, which should be zero.
	rcall xbee_receive_byte
	movwf xbee_receive_length_msb
	xorlw 0
	bnz error_response_bad_length

	; Receive a byte. It should be the LSB of the length, which should be no longer than 100 bytes.
	rcall xbee_receive_byte
	movwf xbee_receive_length_lsb
	movlw 101
	cpfslt xbee_receive_length_lsb
	bra error_response_bad_length

	; Initialize the receive checksum.
	clrf xbee_receive_checksum

	; Set up a receive loop.
	movff xbee_receive_length_lsb, xbee_temp
	lfsr 0, xbee_receive_apiid
xbee_receive_packet_loop:
	rcall xbee_receive_byte
	movwf POSTINC0
	addwf xbee_receive_checksum, F
	decfsz xbee_receive_length_lsb, F
	bra xbee_receive_packet_loop

	; Receive and verify the checksum.
	rcall xbee_receive_byte
	addwf xbee_receive_checksum, W
	xorlw 0xFF
	bnz xbee_receive_packet

	; Deassert RTS to hold off the XBee while processing this packet.
	bsf LAT_RTS, PIN_RTS
	return



xbee_receive_byte:
	; This must only be called from xbee_receive_packet, because it does a neat hack with the return address stack.
	; On success, returns with WREG equal to the received byte.
	; On failure, pops the stack and jumps to xbee_receive_packet.

	; Receive a byte from the USART.
	rcall usart_receive

	; Check if this is an escape.
	xorlw 0x7D
	bz xbee_receive_byte_escaped
	xorlw 0x7D

	; Done!
	return

xbee_receive_byte_escaped:
	; We need to receive another byte and unescape it.
	rcall usart_receive
	xorlw 0x20
	return



usart_receive:
	; This must only be called from xbee_receive_byte, or from xbee_receive_packet with a hack, because the
	; stack upon entry to this function must look like:
	;  ADDRESS OF CALLER OF usart_receive (either xbee_receive_byte or a dummy)
	;  ADDRESS OF CALLER OF CALLER        (xbee_receive_packet)
	;  ADDRESS OF CALLER OF xbee_receive_packet
	; because if an error occurs, the top two levels of the stack are discarded and the function jumps to
	; the entry point of xbee_receive_packet.

	; Check if the bootload pin has been deasserted (gone low).
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	reset

	; Check if the emergency erase pin has been asserted (gone low).
	btfss PORT_EMERG_ERASE, PIN_EMERG_ERASE
	reset

	; Check if there's data available to receive.
	btfss PIR1, RCIF
	bra usart_receive

	; Check for overrun error.
	btfsc RCSTA, OERR
	bra usart_receive_oerr

	; Check for framing error.
	btfsc RCSTA, FERR
	bra usart_receive_ferr

	; Return the byte.
	movf RCREG, W
	return

usart_receive_oerr:
	; Reset the USART.
	bcf RCSTA, CREN
	bsf RCSTA, CREN
	bra usart_receive_error

usart_receive_ferr:
	; Drop the byte.
	movf RCREG, W
	bra usart_receive_error

usart_receive_error:
	; Dump the return addresses and go back to xbee_receive_packet.
	pop
	pop
	bra xbee_receive_packet

	end
