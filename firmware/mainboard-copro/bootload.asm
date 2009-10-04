	; asmsyntax=pic

	; bootload.asm
	; ============
	;
	; This file contains the code to talk to the XBee when in bootloader mode.
	; This allows the FPGA configuration bitstream to be updated over the radio.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "pins.inc"
#include "sleep.inc"
#include "spi.inc"



	; The largest possible packet.
MAX_PAYLOAD equ 100
MAX_PACKET  equ 1 + 8 + 1 + 1 + MAX_PAYLOAD

	; The command identifiers for the commands.
CMD_CHIP_ERASE equ 0x29
CMD_WRITE1     equ 0x48
CMD_WRITE2     equ 0x67
CMD_WRITE3     equ 0x86
CMD_SUM_PAGES  equ 0xA5
CMD_GET_STATUS equ 0xC4
CMD_IDENT      equ 0xE3



	global bootload
	extern configure_fpga



	udata
	; A temporary buffer.
temp: res 1
	; The calculated checksum so far.
computed_sum: res 1
	; The amount of data left to receive.
packet_remaining: res 2
	; The length of the packet, MSB first.
packet_length: res 2
	; The payload of the packet.
packet_data: res MAX_PACKET



writebuf udata
write_buffer: res 256



	code
bootload:
	; Select bank. Never change this in bootload mode.
	banksel packet_length

	; Lock the wheels.
	bcf LAT_BRAKE, PIN_BRAKE

	; Drive PROG_B low to shut down the FPGA.
	bcf LAT_PROG_B, PIN_PROG_B

	; Wait a tenth of a second for the FPGA to shut down.
	call sleep_100ms

	; Take control of the SPI bus.
	call spi_drive

	; Wait for the cleared slave-select lines to settle.
	call sleep_10ms

	; Allow writes to the Flash chip.
	bcf LAT_FLASH_WP, PIN_FLASH_WP

	; Start up the USART.
	clrf RCSTA
	clrf TXSTA
	bsf TRIS_XBEE_TX, PIN_XBEE_TX
	bsf TRIS_XBEE_RX, PIN_XBEE_RX
	clrf SPBRGH
	movlw 68
	movwf SPBRG
	movlw (1 << BRG16)
	movwf BAUDCON
	movlw (1 << TXEN) | (1 << BRGH)
	movwf TXSTA
	movlw (1 << SPEN) | (1 << CREN)
	movwf RCSTA
	nop
	nop
	bra receive_packet



receive_packet:
	; First, wait until we see a delimiter (0x7E).
	rcall receive_byte
	xorlw 0x7E
	bnz receive_packet

receive_packet_post_delimiter:
	; Now receive two more bytes, the length.
	rcall receive_byte
	movwf packet_length + 0
	rcall receive_byte
	movwf packet_length + 1

	; Check that the length is not excessive.
	movlw HIGH(MAX_PACKET)
	cpfsgt packet_length + 0
	bra packet_length_high_ok
	bra receive_packet
packet_length_high_ok:
	cpfseq packet_length + 0
	bra packet_length_ok
	movlw LOW(MAX_PACKET)
	cpfsgt packet_length + 1
	bra packet_length_ok
	bra receive_packet
packet_length_ok:

	; Start receiving bytes.
	lfsr 0, packet_data
	movff packet_length + 0, packet_remaining + 0
	movff packet_length + 1, packet_remaining + 1
	clrf computed_sum
receive_payload_loop:
	; Check for data left to receive.
	movf packet_remaining + 0, F
	bz receive_checksum
	movf packet_remaining + 1, F
	bz receive_checksum

	; Receive a byte of payload.
	rcall receive_byte

	; Store it in the data buffer.
	movwf POSTINC0

	; Add it to the computed checksum.
	addwf computed_sum, F

	; Subtract one from the number of bytes remaining to receive.
	decf packet_remaining + 1, F
	skpc
	decf packet_remaining + 0, F
	bra receive_payload_loop

receive_checksum:
	; Receive the checksum byte.
	rcall receive_byte

	; Add it to the computed checksum.
	addwf computed_sum, F

	; Check if the calculated sum is correct (0xFF).
	movf computed_sum, W
	xorlw 0xFF
	bnz receive_packet

	; Check if the API ID is correct (0x80 = 64-bit receive).
	movf packet_data, W
	xorlw 0x80
	bnz receive_packet

	; We have successfully received a packet. Dispatch based on command ID.
	movf packet_data + 11, W
	xorlw CMD_CHIP_ERASE
	bz erase_chip
	movf packet_data + 11, W
	xorlw CMD_WRITE1
	bz write1
	movf packet_data + 11, W
	xorlw CMD_WRITE2
	bz write2
	movf packet_data + 11, W
	xorlw CMD_WRITE3
	bz write3
	movf packet_data + 11, W
	xorlw CMD_SUM_PAGES
	bz sum_pages
	movf packet_data + 11, W
	xorlw CMD_GET_STATUS
	bz get_status
	movf packet_data + 11, W
	xorlw CMD_IDENT
	skpnz
	bra ident

	; Invalid command.
	bra receive_packet



erase_chip:
	; Send WRITE ENABLE command.
	rcall send_write_enable

	; Send CHIP ERASE command.
	rcall select_chip
	movlw 0xC7
	call spi_send
	rcall deselect_chip

	; Send an ACK immediately. The client will poll the status register.
	bra send_ack



write1:
	; The first byte (at packet_data+11) is the command, CMD_WRITE1.
	; Then (at packet_data+12) are the 99 data bytes.
	; Copy the 99 transmitted data bytes into bytes 0 through 98 of the write buffer.
	lfsr 0, packet_data + 12
	lfsr 1, write_buffer + 0
	movlw 99
write1_loop:
	movff POSTINC0, POSTINC1
	addlw -1
	bnz write1_loop

	; Send an ACK.
	bra send_ack



write2:
	; The first byte (at packet_data+11) is the command, CMD_WRITE2.
	; Then (at packet_data+12) are the 99 data bytes.
	; Copy the 99 transmitted data bytes into bytes 99 through 197 of the write buffer.
	lfsr 0, packet_data + 12
	lfsr 1, write_buffer + 99
	movlw 99
write2_loop:
	movff POSTINC0, POSTINC1
	addlw -1
	bnz write2_loop

	; Send an ACK.
	bra send_ack



write3:
	; The first byte (at packet_data+11) is the command, CMD_WRITE3.
	; The next byte (at packet_data+12) is the MSB of the page number.
	; The next byte (at packet_data+13) is the LSB of the page number.
	; Then (at packet_data+14) are the remaining 58 bytes fo the page (bytes 198 through 255).
	; Copy the remaining data into the write buffer.
	lfsr 0, packet_data + 14
	lfsr 1, write_buffer + 198
	movlw 58
write3_copy_loop:
	movff POSTINC0, POSTINC1
	addlw -1
	bnz write3_copy_loop

	; Send WRITE ENABLE command.
	rcall send_write_enable

	; Send PAGE PROGRAM command with address and data.
	rcall select_chip
	movlw 0x02
	call spi_send
	movf packet_data + 12, W
	call spi_send
	movf packet_data + 13, W
	call spi_send
	movlw 0
	call spi_send
	clrf packet_remaining
	lfsr 0, write_buffer
write3_send_loop:
	movf POSTINC0, W
	call spi_send
	decfsz packet_remaining, F
	bra write3_send_loop
	rcall deselect_chip

	; Wait until STATUS.BUSY clears and then ACK.
	bra wait_until_programmed



sum_pages:
	; The first byte (at packet_data+11) is the command, CMD_PAGE_SUM.
	; The next byte (at packet_data+12) is the MSB of the first page number.
	; The next byte (at packet_data+13) is the LSB of the first page number.

	; We will calculate CRC16s of 32 consecutive pages.

	; Send FAST READ command.
	rcall select_chip
	movlw 0x0B
	call spi_send
	movf packet_data + 12, W
	call spi_send
	movf packet_data + 13, W
	call spi_send
	movlw 0
	call spi_send
	call spi_send

	; Prepare to send the response.
	rcall prepare_xbee_out

	; Let "temp" count the number of pages, and
	; let "packet_remaining" count the number of bytes.
	movlw 32
	movwf temp
	clrf packet_remaining

	; Let FSR0 point at the low byte of the CRC currently
	; being calculated, and FSR1 point at the high byte.
	lfsr 0, packet_data + 11
	lfsr 1, packet_data + 12
sum_pages_outer_loop:
	; Initialize the CRC to 0xFFFF.
	movlw 0xFF
	movwf INDF0
	movwf INDF1

	; Go into a loop.
sum_pages_inner_loop:
	; Update the CRC.
	; crc  = (crc >> 8) | (crc << 8);
	movf INDF0, W
	movff INDF1, INDF0
	movwf INDF1
    ; crc ^= ser_data;
	call spi_receive
	xorwf INDF0, F
    ; crc ^= (crc & 0xff) >> 4;
	swapf INDF0, W
	andlw 0x0F
	xorwf INDF0, F
    ; crc ^= crc << 12;
	swapf INDF0, W
	andlw 0xF0
	xorwf INDF1, F
    ; crc ^= (crc & 0xff) << 5;
	swapf INDF0, W
	rlncf WREG, W
	andlw 0x0F
	xorwf INDF1, F
	swapf INDF0, W
	rlncf WREG, W
	andlw 0xF0
	xorwf INDF0, F

	; Decrement byte count and loop if nonzero.
	decf packet_remaining, F
	bnz sum_pages_inner_loop

	; A page is finished. Advance the FSRs to the next CRC position.
	addfsr 0, 2
	addfsr 1, 2

	; Decrement page count and loop if nonzero.
	decf temp, F
	bnz sum_pages_outer_loop

	; The packet has 64 payload bytes (32 pages × 2 bytes per CRC).
	clrf packet_length + 0
	movlw 64
	movwf packet_length + 1

	; Send response packet over XBee.
	bra send_xbee_packet



get_status:
	; Send READ STATUS REGISTER.
	rcall select_chip
	movlw 0x05
	call spi_send
	call spi_receive
	rcall deselect_chip

	; Send back a length-one packet containing the status register.
	rcall prepare_xbee_out
	movwf packet_data + 11
	clrf packet_length + 0
	movlw 1
	movwf packet_length + 1
	bra send_xbee_packet



ident:
	; Prepare outbound packet with three bytes of payload.
	rcall prepare_xbee_out
	clrf packet_length + 0
	movlw 3
	movwf packet_length + 1

	; Send JEDEC ID.
	rcall select_chip
	movlw 0x9F
	call spi_send
	call spi_receive
	movwf packet_data + 11
	call spi_receive
	movwf packet_data + 12
	call spi_receive
	movwf packet_data + 13
	rcall deselect_chip

	; Transmit the packet.
	bra send_xbee_packet



wait_until_programmed:
	; Send READ STATUS REGISTER until bit 0 (BUSY) is clear.
	rcall select_chip
	movlw 0x05
	call spi_send
wait_until_programmed_loop:
	call spi_receive
	andlw 1
	bnz wait_until_programmed_loop
	rcall deselect_chip
	bra send_ack



send_ack:
	; Send back a zero-length packet over the radio indicating success.
	rcall prepare_xbee_out
	clrf packet_length + 0
	clrf packet_length + 1
	bra send_xbee_packet



receive_byte:
	; Receive a byte.
	rcall receive_byte_raw
	; Check if it's a delimiter.
	xorlw 0x7E
	skpnz
	bra receive_packet_post_delimiter
	; Check if it's an escape code.
	xorlw 0x7D ^ 0x7E
	bz receive_byte_escaped
	; Return the byte.
	xorlw 0x7D
	return
receive_byte_escaped:
	; Receive a byte.
	rcall receive_byte_raw
	; Check if it's a delimiter.
	xorlw 0x7E
	skpnz
	bra receive_packet_post_delimiter
	; Check if it's an escape code.
	xorlw 0x7D ^ 0x7E
	bz receive_byte_escaped
	; Return the byte, unescaping it.
	xorlw 0x20 ^ 0x7D
	return



receive_byte_raw:
	; If RCIF=1, break out of the loop.
	btfsc PIR1, RCIF
	bra receive_byte_rcif
	; If BOOTLOAD=0, let the error handler tidy up.
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	bra serial_error
	; Loop back.
	bra receive_byte
receive_byte_rcif:
	; Check for an error.
	movf RCSTA, W
	andlw (1 << OERR) | (1 << FERR)
	bnz serial_error
	; Return with the byte in WREG.
	movf RCREG, W
	return



serial_error:
	; Something went wrong. Either there's a framing or overrun error on the
	; serial port, the XBee packet had an incorrect length or checksum, the
	; packet contained an unrecognized command ID, or the bootload pin went low
	; while receiving the byte.

	; This label is jumped to from inside receive_byte_raw, which is called from
	; receive_byte, which is called from mainline code. Fix up the stack.
	pop
	pop

	; First, check if the bootload pin went low.
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	bra bootload_done

	; Check for an overrun error, which can be cleared by restarting the
	; receiver.
	btfsc RCSTA, OERR
	bcf RCSTA, CREN
	bsf RCSTA, CREN

	; Check for a framing error, which can be cleared by reading a byte.
	btfsc RCSTA, FERR
	movf RCREG, W

	; At this point, we know that the bootloader pin is still high and that the
	; serial port has been reinitialized if needed. We should now go back and
	; try to listen for another packet.
	bra receive_packet



bootload_done:
	; The XBee bootload signal line has gone low.
	; Shut down the USART.
	clrf RCSTA
	clrf TXSTA
	bsf TRIS_XBEE_TX, PIN_XBEE_TX
	bsf TRIS_XBEE_RX, PIN_XBEE_RX

	; Write-protect the Flash chip.
	bsf LAT_FLASH_WP, PIN_FLASH_WP

	; Start configuring the FPGA.
	goto configure_fpga



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



prepare_xbee_out:
	; In a 64-bit receive packet, the source address is at offset 1.
	; In a 64-bit transmit packet, the destination address is at offset 2.
	; So copy the address over.
	lfsr 0, packet_data + 1 + 7
	lfsr 1, packet_data + 2 + 7
	movff POSTDEC0, POSTDEC1
	movff POSTDEC0, POSTDEC1
	movff POSTDEC0, POSTDEC1
	movff POSTDEC0, POSTDEC1
	movff POSTDEC0, POSTDEC1
	movff POSTDEC0, POSTDEC1
	movff POSTDEC0, POSTDEC1
	movff POSTDEC0, POSTDEC1

	; Set a frame ID of zero because we don't want to deal with status frames.
	clrf packet_data + 1

	; Set options to 0 (do radio-level ACKs, do not do broadcast).
	clrf packet_data + 10

	; Set API ID to 0x00 (64-bit transmit).
	clrf packet_data + 0
	return



send_xbee_packet:
	; Add the 11 bytes of overhead to the packet length.
	movlw 11
	addwf packet_length + 1, F
	movlw 0
	addwfc packet_length + 0, F

	; Send delimiter.
	movlw 0x7E
	rcall send_xbee_byte_raw

	; Send length.
	movf packet_length + 0
	rcall send_xbee_byte
	movf packet_length + 1
	rcall send_xbee_byte

	; Send payload and compute checksum as we go.
	movff packet_length + 0, packet_remaining + 0
	movff packet_length + 1, packet_remaining + 1
	lfsr 0, packet_data
	clrf computed_sum
send_xbee_packet_payload_loop:
	; Check if there's data left to send.
	movf packet_length + 0, F
	bz send_xbee_packet_payload_done
	movf packet_length + 1, F
	bz send_xbee_packet_payload_done

	; Send a byte and compute checksum as we go.
	movf POSTINC0, W
	addwf computed_sum, F
	rcall send_xbee_byte

	; Decrement byte count.
	decf packet_length + 1, F
	skpc
	decf packet_length + 0, F
	bra send_xbee_packet_payload_loop
send_xbee_packet_payload_done:

	; Do checksum fixup and send.
	bsf STATUS, C
	movlw 0xFF
	subfwb computed_sum, W
	rcall send_xbee_byte

	; Done!
	goto receive_packet



send_xbee_byte:
	; Check if it needs to be escaped.
	xorlw 0x7E
	bz send_xbee_byte_7e
	xorlw 0x7D ^ 0x7E
	bz send_xbee_byte_7d
	xorlw 0x11 ^ 0x7D
	bz send_xbee_byte_11
	xorlw 0x13 ^ 0x11
	bz send_xbee_byte_13
	xorlw 0x13
	bra send_xbee_byte_raw

send_xbee_byte_7e:
	movlw 0x7D
	rcall send_xbee_byte_raw
	movlw 0x7E ^ 0x20
	bra send_xbee_byte_raw

send_xbee_byte_7d:
	movlw 0x7D
	rcall send_xbee_byte_raw
	movlw 0x7D ^ 0x20
	bra send_xbee_byte_raw

send_xbee_byte_11:
	movlw 0x7D
	rcall send_xbee_byte_raw
	movlw 0x11 ^ 0x20
	bra send_xbee_byte_raw

send_xbee_byte_13:
	movlw 0x7D
	rcall send_xbee_byte_raw
	movlw 0x13 ^ 0x20
	bra send_xbee_byte_raw

send_xbee_byte_raw:
	movwf TXREG
send_xbee_byte_raw_wait:
	btfss TXSTA, TRMT
	bra send_xbee_byte_raw_wait
	return

	end
