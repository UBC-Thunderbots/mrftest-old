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
MAX_PAYLOAD equ 1 + 2 + 256
MAX_PACKET  equ 1 + 8 + 1 + 1 + MAX_PAYLOAD

	; The command identifiers for the commands.
CMD_CHIP_ERASE equ 0x27
CMD_PAGE_WRITE equ 0x73
CMD_PAGE_SUM   equ 0x5A



	global bootload
	extern configure_fpga



xbeebuf udata
	; The calculated checksum so far.
computed_sum: res 1
	; The amount of data left to receive.
packet_remaining: res 2
	; The length of the packet, MSB first.
packet_length: res 2
	; The payload of the packet.
packet_data: res MAX_PACKET



	code
bootload:
	; Select data bank.
	banksel packet_remaining

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
	addlw 0
	bz serial_error
	movf RCREG, W
	xorlw 0x7E
	bnz receive_packet

	; Now receive two more bytes, the length.
	rcall receive_byte
	addlw 0
	bz serial_error
	movf RCREG, W
	movwf packet_length + 0
	rcall receive_byte
	addlw 0
	bz serial_error
	movf RCREG, W
	movwf packet_length + 1

	; Check that the length is not excessive.
	movlw HIGH(MAX_PACKET)
	cpfsgt packet_length + 0
	bra packet_length_high_ok
	bra serial_error
packet_length_high_ok:
	cpfseq packet_length + 0
	bra packet_length_ok
	movlw LOW(MAX_PACKET)
	cpfsgt packet_length + 1
	bra packet_length_ok
	bra serial_error
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
	addlw 0
	bz serial_error
	movf RCREG, W

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
	addlw 0
	bz serial_error
	movf RCREG, W

	; Add it to the computed checksum.
	addwf computed_sum, F

	; Check if the calculated sum is correct (0xFF).
	movf computed_sum, W
	xorlw 0xFF
	bnz serial_error

	; Check if the API ID is correct (0x80 = 64-bit receive).
	movf packet_data, W
	xorlw 0x80
	bnz serial_error

	; We have successfully received a packet. Dispatch based on command ID.
	movf packet_data + 11, W
	xorlw CMD_CHIP_ERASE
	bz erase_chip
	movf packet_data, W
	xorlw CMD_PAGE_WRITE
	bz write_page
	movf packet_data, W
	xorlw CMD_PAGE_SUM
	bz sum_page

	; Invalid command.
	bra serial_error



serial_error:
	; Something went wrong. Either there's a framing or overrun error on the
	; serial port, the XBee packet had an incorrect length or checksum, the
	; packet contained an unrecognized command ID, or the bootload pin went low
	; while receiving the byte.
	; First, check if the bootload pin went low.
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	bra bootload_done

	; Check for an overrun error, which can be cleared by restarting the
	; receiver.
	btfsc RCSTA, OERR
	bcf RCSTA, CREN
	btfss RCSTA, CREN
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



erase_chip:
	; Send WRITE ENABLE command.
	rcall send_write_enable

	; Send CHIP ERASE command.
	rcall select_chip
	movlw 0xC7
	call spi_send
	rcall deselect_chip

	; Wait until STATUS.BUSY clears.
	bra wait_until_programmed



write_page:
	; Send WRITE ENABLE command.
	rcall send_write_enable

	; Send PAGE PROGRAM command with address and data.
	rcall select_chip
	movlw 0x02
	call spi_send
	movf packet_data + 1, W
	call spi_send
	movf packet_data + 2, W
	call spi_send
	movlw 0
	call spi_send
	clrf packet_remaining
	lfsr 0, packet_data + 12
write_page_loop:
	movf POSTINC0, W
	call spi_send
	decfsz packet_remaining
	bra write_page_loop
	rcall deselect_chip

	; Wait until STATUS.BUSY clears.
	bra wait_until_programmed



sum_page:
	; Send FAST READ command and read in 256 bytes of data.
	rcall select_chip
	movlw 0x0B
	call spi_send
	movf packet_data + 0, W
	call spi_send
	movf packet_data + 1, W
	call spi_send
	movlw 0
	call spi_send
	movlw 0
	call spi_send
	clrf packet_remaining
	clrf computed_sum
sum_page_loop:
	call spi_receive
	addwf computed_sum, F
	decfsz packet_remaining
	bra sum_page_loop
	rcall deselect_chip

	; Send response packet over XBee.
	rcall prepare_xbee_out
	movff computed_sum, packet_data + 11
	clrf packet_length + 0
	movlw 1
	movwf packet_length + 1
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

	; Send back a zero-length packet over the radio indicating success.
	rcall prepare_xbee_out
	clrf packet_length + 0
	clrf packet_length + 1
	bra send_xbee_packet



receive_byte:
	btfsc PIR1, RCIF
	bra receive_byte_rcif
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	retlw 0
	bra receive_byte
receive_byte_rcif:
	movf RCSTA, W
	andlw (1 << OERR) | (1 << FERR)
	skpz
	retlw 0
	retlw 1



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
	clrf packet_data + 0

	; Set options to 0 (do radio-level ACKs, do not do broadcast).
	clrf packet_data + 10

	; Set API ID to 0x00 (64-bit transmit).
	clrf packet_data + 0
	return



send_xbee_packet:
	; Send delimiter.
	movlw 0x7E
	rcall send_xbee_byte

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
	movwf TXREG
send_xbee_byte_wait:
	btfss TXSTA, TRMT
	bra send_xbee_byte_wait
	return
	end
