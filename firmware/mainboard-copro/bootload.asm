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
	;  1 byte command ID + index
	;    The upper 4 bits are one of the COMMAND_* constants.
	;    The lower 4 bits are a command-specific small numerical parameter.
	;  n bytes command data
	;
	; The bootloader receives the command, handles it, and sends back a
	; RESPONSE. The RESPONSE packet has this form:
	;  1 byte command ID + index
	;    This byte has the same value as the corresponding byte in the command
	;    packet.
	;  1 byte command status
	;    This byte contains one of the COMMAND_STATUS_* constants.
	;  n bytes response data (only if status is COMMAND_STATUS_OK)
	;
	; The host is expected to only send another command after receiving the
	; previous command's response.
	;
	;
	;
	; In order to improve the performance of long-running IO operations (Flash
	; erases, writes, and CRCs), IO operations can be parallelized. Rather than
	; simply sending a command packet to execute an IO operation and then
	; waiting for it to finish, the host can enqueue multiple operations and
	; check their status later. This is done through the use of IO REQUEST
	; PACKETS. There are four IRPs, each having the following form:
	;  1 byte status+operation:
	;    The upper 4 bits are one of the IRP_STATUS_* constants.
	;    The lower 4 bits are one of the IO_OPERATION_* constants.
	;  1 byte MSB of page number to address
	;  1 byte LSB of page number to address
	;
	; Associated with each IRP is a BUFFER. A buffer is a block of 256 bytes of
	; storage space which can be used by an IO operation enqueued in the
	; corresponding IRP. For example, a write operation will write the data in
	; its corresponding buffer into the Flash chip. A CRC operation will read
	; the requested region from the Flash chip and store the generated page CRCs
	; into the buffer. Erase operations do not use buffer space. The buffer
	; associated with an IRP can be read and written by the host.
	;
	; It is expected that the host will treat the IRPs like a FIFO, keeping a
	; write pointer into the IRP array and submitting each new operation at the
	; next position in the array.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "dbgprint.inc"
#include "dispatch.inc"
#include "pins.inc"
#include "sleep.inc"
#include "spi.inc"



	global bootload
	global rcif_main
	global txif_main



	; COMMAND_IDENT
	; =============
	;
	; Requests an ID string from the bootloader and Flash chip. This command can
	; be submitted at any time; it does not actually talk to the Flash chip (the
	; JEDEC ID is cached at bootloader initialization).
	;
	; Index number:
	;  Ignored
	;
	; Request data:
	;  None
	;
	; Statii:
	;  COMMAND_STATUS_OK
	;   this command always succeeds
	;
	; Response data:
	;  5 bytes ASCII string 'TBOTS'
	;  1 byte JEDEC manufacturer ID of Flash chip
	;  1 byte JEDEC memory type ID of Flash chip
	;  1 byte JEDEC capacity ID of Flash chip
	;
COMMAND_IDENT equ 0x1

	; COMMAND_READ_BUFFER
	; ===================
	;
	; Requests that the bootloader send to the host part of the contents of a
	; buffer.
	;
	; Index number:
	;  Buffer number (must be in the range [0,3])
	;
	; Request data:
	;  1 byte offset in buffer at which to start reading
	;  1 byte number of bytes to read (must be in the range [0,98])
	;
	; Statii:
	;  COMMAND_STATUS_OK
	;   if the read was successful
	;  COMMAND_STATUS_BAD_INDEX
	;   if the index number was not in the range [0,3]
	;  COMMAND_STATUS_BAD_LENGTH
	;   if the length was not in the range [0,98], or if the read request would
	;   overrun the end of the buffer
	;
	; Response data:
	;  n bytes buffer contents
	;
COMMAND_READ_BUFFER equ 0x2

	; COMMAND_WRITE_BUFFER
	; ====================
	;
	; Writes a block of data into a buffer.
	;
	; Index number:
	;  Buffer number (must be in the range [0,3])
	;
	; Request data:
	;  1 byte offset in buffer at which to start writing
	;  n bytes data to write into buffer (remainder of XBee packet)
	;
	; Statii:
	;  COMMAND_STATUS_OK
	;   if the write was successful
	;  COMMAND_STATUS_BAD_INDEX
	;   if the index number was not in the range [0,3]
	;  COMMAND_STATUS_BAD_LENGTH
	;   if the read request would overrun the end of the buffer
	;
	; Response data:
	;  None
	;
COMMAND_WRITE_BUFFER equ 0x3

	; COMMAND_READ_IRPS
	; =================
	;
	; Requests that the bootloader send to the host the status+operation bytes
	; of the IO request packets.
	;
	; Index number:
	;  Ignored
	;
	; Request data:
	;  None
	;
	; Statii:
	;  COMMAND_STATUS_OK
	;   this command always succeeds
	;
	; Response data:
	;  4 bytes status+operation bytes of the IRPs
	;
COMMAND_READ_IRPS equ 0x4

	; COMMAND_CLEAR_IRPS
	; ==================
	;
	; Clears completed, errored, or pending IRPs. Requesting to clear an empty
	; or busy IRP will silently do nothing.
	;
	; Index number:
	;  The bitmask of IRPs to clear
	;
	; Request data:
	;  None
	;
	; Statii:
	;  COMMAND_STATUS_OK
	;   this command always succeeds
	;
	; Response data:
	;  None
	;
COMMAND_CLEAR_IRPS equ 0x5

	; COMMAND_SUBMIT_IRP
	; ==================
	;
	; Submits a new IRP. The specified IRP must be empty when this packet is
	; sent. To overwrite a nonempty IRP, clear it first. Note that the list of
	; statii for this command only reflects errors in the submission command
	; itself. Errors in the submitted IRP are reported asynchronously through
	; the IRP status+operation byte.
	;
	; Index number:
	;  The index of the IRP to submit
	;
	; Request data:
	;  1 byte operation number, one of the IO_OPERATION_* constants
	;  1 byte MSB of page number to address
	;  1 byte LSB of page number to address
	;
	; Statii:
	;  COMMAND_STATUS_OK
	;   if the IRP has been queued
	;  COMMAND_STATUS_BAD_INDEX
	;   if the index number was not in the range [0,3]
	;  COMMAND_STATUS_IRP_IN_USE
	;   if the IRP was not empty
	;
	; Response data:
	;  None
	;
COMMAND_SUBMIT_IRP equ 0x6



	; IO_OPERATION_ERASE_BLOCK
	; ========================
	;
	; Erases a 64kB block of the Flash chip.
	;
	; Address:
	;  The address word must specify the first page in the block to erase. The
	;  page number must therefore be a multiple of 256.
	;
	; Completion Statii:
	;  IRP_STATUS_COMPLETE
	;   if the operation completed successfully
	;  IRP_STATUS_BAD_ADDRESS
	;   if the page number is illegal for this operation
	;
IO_OPERATION_ERASE_BLOCK equ 0x1

	; IO_OPERATION_WRITE_PAGE
	; =======================
	;
	; Writes a 256 byte page of the Flash chip. The data written to the chip
	; will come from the IRP's associated buffer.
	;
	; Address:
	;  The address word must specify the page to write.
	;
	; Completion Statii:
	;  IRP_STATUS_COMPLETE
	;   if the operation completed successfully
	;  IRP_STATUS_BAD_ADDRESS
	;   if the page number is illegal for this operation
	;
IO_OPERATION_WRITE_PAGE equ 0x2

	; IO_OPERATION_CRC_SECTOR
	; =======================
	;
	; Calculates the CRC16s of the pages making up a 4kB sector. There are 16
	; 256-byte pages in a sector; for each page, a CRC will be calculated and
	; stored in the IRP's associated buffer.
	;
	; Address:
	;  The address word must specify the first page of the sector to CRC. The
	;  page number must thus be a multiple of 16.
	;
	; Completion Statii:
	;  IRP_STATUS_COMPLETE
	;   if the operation completed successfully
	;  IRP_STATUS_BAD_ADDRESS
	;   if the page number is illegal for this operation
	;
IO_OPERATION_CRC_SECTOR equ 0x3



	; COMMAND_STATUS_OK
	; =================
	;
	; Returned when a command was executed successfully.
	;
COMMAND_STATUS_OK         equ 0x00

	; COMMAND_STATUS_BAD_INDEX
	; ========================
	;
	; Returned when the index number associated with a command was illegal.
	;
COMMAND_STATUS_BAD_INDEX  equ 0x01

	; COMMAND_STATUS_BAD_LENGTH
	; =========================
	;
	; Returned when a length value associated with a command was too large.
	;
COMMAND_STATUS_BAD_LENGTH equ 0x02

	; COMMAND_STATUS_IRP_IN_USE
	; =========================
	;
	; Returned when a command attempts to submit to a nonempty IRP.
	;
COMMAND_STATUS_IRP_IN_USE equ 0x03

	; COMMAND_STATUS_BAD_COMMAND
	; ==========================
	;
	; Returned when an invalid command ID is sent.
	;
COMMAND_STATUS_BAD_COMMAND equ 0x04



	; IRP_STATUS_EMPTY
	; ================
	;
	; Present when an IRP is clear and can accept a new submission.
	;
IRP_STATUS_EMPTY equ 0x0

	; IRP_STATUS_PENDING
	; ==================
	;
	; Present when an IRP has been submitted but the IO operation has not yet
	; started.
	;
IRP_STATUS_PENDING equ 0x1

	; IRP_STATUS_BUSY
	; ===============
	;
	; Present when an IRP is currently being executed.
	;
IRP_STATUS_BUSY equ 0x2

	; IRP_STATUS_COMPLETE
	; ===================
	;
	; Present when the IO operation has finished.
	;
IRP_STATUS_COMPLETE equ 0x3

	; IRP_STATUS_BAD_OPERATION
	; ========================
	;
	; Present when the IO operation code is not recognized.
	;
IRP_STATUS_BAD_OPERATION equ 0x4

	; IRP_STATUS_BAD_ADDRESS
	; ======================
	;
	; Present when the address is illegal for the specified operation.
	;
IRP_STATUS_BAD_ADDRESS equ 0x5



	; Allocate space for the IO buffers.
buffer0 udata
buf0: res 256

buffer1 udata
buf1: res 256

buffer2 udata
buf2: res 256

buffer3 udata
buf3: res 256



	; Allocate space for the IRPs.
irps udata
irp0: res 3
irp1: res 3
irp2: res 3
irp3: res 3



	; Allocate space for XBee packets.
xbeeinbuf udata
xbee_in_length: res 1
xbee_in_apiid: res 1
xbee_in_address: res 8
xbee_in_rssi: res 1
xbee_in_options: res 1
xbee_in_payload:
xbee_in_commandid: res 1
XBEE_IN_COMMANDDATA_OFFSET equ $-xbee_in_apiid
xbee_in_commanddata: res 99
XBEE_IN_MAX_LENGTH equ $-xbee_in_apiid
xbee_in_ptr: res 1
xbee_in_escape: res 1

xbeeoutbuf udata
xbee_out_length: res 1
xbee_out_apiid: res 1
xbee_out_frameid: res 1
xbee_out_address: res 8
xbee_out_options: res 1
xbee_out_payload:
xbee_out_commandid: res 1
xbee_out_commandstatus: res 1
xbee_out_responsedata: res 98
xbee_out_ptr: res 1
xbee_out_escape: res 1
XBEE_OUT_RESPONSEDATA_OFFSET equ xbee_out_responsedata - xbee_out_apiid
XBEE_OUT_MAX_LENGTH equ xbee_out_ptr - xbee_out_apiid



	; Allocate space for miscellaneous variables.
miscdata udata
jedecid: res 3
irpptr: res 1
bytecounter: res 1
pagecounter: res 1
temp: res 1



	; Register allocation rules
	; =========================
	;
	; FSR0 and FSR1 are used only by interrupt handlers.
	; FSR2 is used only by the mainline.
	;



	code
	; Handles a receive interrupt.
rcif_main:
	; Select the appropriate bank.
	banksel xbee_in_length

	; Check the receiver status for an error condition.
	movf RCSTA, W
	andlw (1 << FERR) | (1 << OERR)
	bnz rcif_serial_error

	; Check the receive pointer.
	movf xbee_in_ptr, W
	DISPATCH_INIT
	DISPATCH_COND 0xFF, rcif_waiting_for_sop
	DISPATCH_COND 0xFE, rcif_waiting_for_length_msb
	DISPATCH_COND 0xFD, rcif_waiting_for_length_lsb
	DISPATCH_END_RESTORE
	; Check if it's equal to length, meaning waiting for checksum.
	xorwf xbee_in_length, W
	bz rcif_waiting_for_checksum

	; It's none of the special values, meaning we're receiving payload.
	; Address the appropriate byte in the buffer.
	lfsr 0, xbee_in_apiid
	movf xbee_in_ptr, W
	addwf FSR0L, F
	; Receive the byte.
	movf RCREG, W
	; Check for special values.
	DISPATCH_INIT
	DISPATCH_COND 0x7E, rcif_sop
	DISPATCH_COND 0x7D, rcif_escape
	DISPATCH_END_RESTORE
	; Unescape the byte if needed.
	tstfsz xbee_in_escape
	xorlw 0x20
	clrf xbee_in_escape
	; Store the byte into the buffer.
	movwf INDF0
	; Increment the pointer.
	incf xbee_in_ptr, F
	return

rcif_waiting_for_sop:
	; We're waiting to see an SOP byte. That's the only thing we care about.
	movf RCREG, W
	xorlw 0x7E
	bz rcif_sop
	return

rcif_sop:
	; Got SOP. Mark the pointer with the special "waiting for length MSB" value.
	movlw 0xFE
	movwf xbee_in_ptr
	clrf xbee_in_escape
	return

rcif_wait_for_sop:
	; We have decided we want to wait for the SOP. Mark the pointer as such.
	setf xbee_in_ptr
	return

rcif_escape:
	; The escape byte was received. Set the flag so we're ready for the next
	; byte.
	setf xbee_in_escape
	return

rcif_waiting_for_length_msb:
	; We're waiting for the MSB of the length word. This should always be zero,
	; because we don't understand very long packets. Since the byte is zero, it
	; doesn't need escaping, which means we should never seen an escape sequence
	; here. So any nonzero byte is bad news and should cause us to give up and
	; go back to waiting for the SOP.
	movf RCREG, W
	DISPATCH_INIT
	DISPATCH_COND 0x7E, rcif_sop
	DISPATCH_END_RESTORE
	bnz rcif_wait_for_sop
	; The byte was zero. Now we're waiting for the LSB of the length.
	movlw 0xFD
	movwf xbee_in_ptr
	return

rcif_waiting_for_length_lsb:
	; We're waiting for the LSB of the length word. This one might be escaped,
	; so we need to do escape processing.
	movf RCREG, W
	DISPATCH_INIT
	DISPATCH_COND 0x7E, rcif_sop
	DISPATCH_COND 0x7D, rcif_escape
	DISPATCH_END_RESTORE
	tstfsz xbee_in_escape
	xorlw 0x20
	clrf xbee_in_escape
	; We have the length byte. Store it.
	movwf xbee_in_length
	; Check if it's within the range of lengths we understand.
	movlw XBEE_IN_MAX_LENGTH + 1
	cpfslt xbee_in_length
	bra rcif_wait_for_sop
	movf xbee_in_length, W
	bz rcif_wait_for_sop
	; The length is OK. Set the pointer to zero to prepare for the payload.
	clrf xbee_in_ptr
	return

rcif_waiting_for_checksum:
	; We're waiting for the checksum. This might be escaped.
	movf RCREG, W
	DISPATCH_INIT
	DISPATCH_COND 0x7E, rcif_sop
	DISPATCH_COND 0x7D, rcif_escape
	DISPATCH_END_RESTORE
	tstfsz xbee_in_escape
	xorlw 0x20
	clrf xbee_in_escape
	; Sum up the checksum plus the payload which was received earlier.
	lfsr 0, xbee_in_apiid
	movff xbee_in_length, xbee_in_ptr
rcif_checksum_loop:
	addwf POSTINC0, W
	decf xbee_in_ptr, F
	bnz rcif_checksum_loop
	; Check if the checksum is valid.
	xorlw 0xFF
	bnz rcif_wait_for_sop
	; Handle the received packet
	bra rcif_handle_packet

rcif_serial_error:
	; An error occurred on the serial port.
	; If we have an overrun error, reset the receiver.
	btfsc RCSTA, OERR
	bcf RCSTA, CREN
	bsf RCSTA, CREN

	; If we have a framing error, receive and discard the byte to clear it.
	btfsc RCSTA, FERR
	movf RCSTA, W

	; Go wait for a SOP.
	bra rcif_wait_for_sop

rcif_handle_packet:
	; What we want to do after this is wait for a SOP. Mark that now, so we
	; don't have to keep branching later on.
	setf xbee_in_ptr

	; Check if the API ID is 0x80 ("RX Packet: 64-bit Address")
	movf xbee_in_apiid, W
	xorlw 0x80
	skpz
	return

	; For now, xbee_out_length is the length of the response data. Zero it.
	movlw 0
	movff WREG, xbee_out_length

	; Dispatch the received packet based on command ID.
	swapf xbee_in_commandid, W
	andlw 0x0F
	DISPATCH_INIT
	DISPATCH_COND COMMAND_IDENT, rcif_handle_ident
	DISPATCH_COND COMMAND_READ_BUFFER, rcif_handle_read_buffer
	DISPATCH_COND COMMAND_WRITE_BUFFER, rcif_handle_write_buffer
	DISPATCH_COND COMMAND_READ_IRPS, rcif_handle_read_irps
	DISPATCH_COND COMMAND_CLEAR_IRPS, rcif_handle_clear_irps
	DISPATCH_COND COMMAND_SUBMIT_IRP, rcif_handle_submit_irp
	DISPATCH_END_NORESTORE

	; Bad command. Enqueue BAD COMMAND response.
	movlw COMMAND_STATUS_BAD_COMMAND
	bra rcif_queue_response

rcif_handle_ident:
	; We got the IDENT command. Send back the IDENT string.
	banksel xbee_out_length
	movlw 8
	movwf xbee_out_length
	movlw 'T'
	movwf xbee_out_responsedata + 0
	movlw 'B'
	movwf xbee_out_responsedata + 1
	movlw 'O'
	movwf xbee_out_responsedata + 2
	movlw 'T'
	movwf xbee_out_responsedata + 3
	movlw 'S'
	movwf xbee_out_responsedata + 4
	movff jedecid + 0, xbee_out_responsedata + 5
	movff jedecid + 1, xbee_out_responsedata + 6
	movff jedecid + 2, xbee_out_responsedata + 7
	bra rcif_queue_command_status_ok

rcif_handle_read_buffer:
	; We got the READ BUFFER command. Validate buffer number.
	movlw 0x0C
	andwf xbee_in_commandid, W
	bnz rcif_queue_command_status_bad_index
	; Validate length.
	movlw 99
	cpfslt xbee_in_commanddata + 1
	bra rcif_queue_command_status_bad_length
	movf xbee_in_commanddata + 0
	addwf xbee_in_commanddata + 1, W
	bc rcif_queue_command_status_bad_length
	; Prepare response.
	rcall rcif_address_buffer_fsr0
	movf xbee_in_commanddata + 0
	addwf FSR0L, F
	movff xbee_in_commanddata + 1, xbee_out_length
	lfsr 1, xbee_out_responsedata
rcif_handle_read_buffer_loop:
	movff POSTINC0, POSTINC1
	decf xbee_in_commanddata + 1
	bnz rcif_handle_read_buffer_loop
	; Send response.
	bra rcif_queue_command_status_ok

rcif_handle_write_buffer:
	; We got the WRITE BUFFER command. Validate buffer number.
	movlw 0x0C
	andwf xbee_in_commandid, W
	bnz rcif_queue_command_status_bad_index
	; Validate offset/length.
	movf xbee_in_length, W
	addlw -XBEE_IN_COMMANDDATA_OFFSET - 1
	addwf xbee_in_commanddata + 0
	bc rcif_queue_command_status_bad_length
	; Copy data to buffer.
	rcall rcif_address_buffer_fsr0
	movf xbee_in_commanddata + 0
	addwf FSR0L, F
	lfsr 1, xbee_in_commanddata + 1
	movf xbee_in_length, W
	addlw -XBEE_IN_COMMANDDATA_OFFSET - 1
	bz rcif_queue_command_status_bad_length
rcif_handle_write_buffer_loop:
	movff POSTINC1, POSTINC0
	addlw -1
	bnz rcif_handle_write_buffer_loop
	; Send response.
	bra rcif_queue_command_status_ok

rcif_handle_read_irps:
	; We got the READ IRPS command. Copy the response data.
	movff irp0 + 0, xbee_out_responsedata + 0
	movff irp1 + 0, xbee_out_responsedata + 1
	movff irp2 + 0, xbee_out_responsedata + 2
	movff irp3 + 0, xbee_out_responsedata + 3
	; Send the response.
	movlw 4
	movff WREG, xbee_out_length
	bra rcif_queue_command_status_ok

rcif_handle_clear_irps:
	; We got the CLEAR IRPS command. Go through the IRPs.
	lfsr 0, irp0
IRP = 0
	while IRP < 4
	; Check if we've been asked to clear this IRP.
	btfss xbee_in_commandid, IRP
	bra rcif_handle_clear_irps_skip_#v(IRP)
	; Check if this IRP is in a clearable state (anything except BUSY).
	swapf INDF0, W
	andlw 0x0F
	xorlw IRP_STATUS_BUSY
	bz rcif_handle_clear_irps_skip_#v(IRP)
	; Clear the IRP.
	clrf INDF0
rcif_handle_clear_irps_skip_#v(IRP)
if IRP < 3
	; Advance to the next IRP.
	addfsr 0, 3
endif
IRP = IRP + 1
	endw
	; Send the response.
	bra rcif_queue_command_status_ok

rcif_handle_submit_irp:
	; We got the SUBMIT IRP command. Validate IRP number.
	movlw 0x0C
	andwf xbee_in_commandid, W
	bnz rcif_queue_command_status_bad_index
	; Point FSR0 at the IRP.
	lfsr 0, irp0
	movf xbee_in_commandid, W
	andlw 0x07
	addwf FSR0L, F
	addwf FSR0L, F
	addwf FSR0L, F
	; Check that the IRP is empty.
	tstfsz INDF0
	bra rcif_queue_command_status_irp_in_use
	; Fill in the IRP.
	movf xbee_in_commanddata + 0, W
	andlw 0x0F
	movwf POSTINC0
	movff xbee_in_commanddata + 1, POSTINC0
	movff xbee_in_commanddata + 2, POSTINC0
	; Send response.
	bra rcif_queue_command_status_ok

rcif_queue_command_status_ok:
	movlw COMMAND_STATUS_OK
	bra rcif_queue_response

rcif_queue_command_status_bad_index:
	movlw COMMAND_STATUS_BAD_INDEX
	bra rcif_queue_response

rcif_queue_command_status_bad_length:
	movlw COMMAND_STATUS_BAD_LENGTH
	bra rcif_queue_response

rcif_queue_command_status_irp_in_use:
	movlw COMMAND_STATUS_IRP_IN_USE
	bra rcif_queue_response

rcif_queue_response:
	; Set command ID and status.
	banksel xbee_out_commandstatus
	movwf xbee_out_commandstatus
	movff xbee_in_commandid, xbee_out_commandid
	; responsedata has been filled by the caller.
	; length has been filled with number of bytes in responsedata.
	; Need to add XBEE_OUT_RESPONSEDATA_OFFSET so we get the rest of the packet.
	movlw XBEE_OUT_RESPONSEDATA_OFFSET
	addwf xbee_out_length, F
	movf xbee_out_length, W
	; Set up the rest of the outbound packet.
	clrf xbee_out_apiid
	clrf xbee_out_frameid
	clrf xbee_out_options
	movff xbee_in_address + 0, xbee_out_address + 0
	movff xbee_in_address + 1, xbee_out_address + 1
	movff xbee_in_address + 2, xbee_out_address + 2
	movff xbee_in_address + 3, xbee_out_address + 3
	movff xbee_in_address + 4, xbee_out_address + 4
	movff xbee_in_address + 5, xbee_out_address + 5
	movff xbee_in_address + 6, xbee_out_address + 6
	movff xbee_in_address + 7, xbee_out_address + 7
	; Set the pointer to 0xFF to tell the transmit ISR it needs to send SOP.
	setf xbee_out_ptr
	; Enable transmit interrupts so our packet will be sent.
	bsf PIE1, TXIE
	; Return from interrupt; allow transmit ISR to work.
	return

rcif_address_buffer_fsr0:
	; Use the index number in xbee_in_commandid to select a buffer.
	movf xbee_in_commandid, W
	andlw 0x07
	DISPATCH_INIT
	DISPATCH_COND 0, rcif_address_buffer_fsr0_0
	DISPATCH_COND 1, rcif_address_buffer_fsr0_1
	DISPATCH_COND 2, rcif_address_buffer_fsr0_2
	DISPATCH_COND 3, rcif_address_buffer_fsr0_3
	DISPATCH_END_NORESTORE

rcif_address_buffer_fsr0_0:
	lfsr 0, buf0
	return

rcif_address_buffer_fsr0_1:
	lfsr 0, buf1
	return

rcif_address_buffer_fsr0_2:
	lfsr 0, buf2
	return

rcif_address_buffer_fsr0_3:
	lfsr 0, buf3
	return



txif_main:
	; Transmitter ready interrupt handler.
	; Check that length is nonzero.
	banksel xbee_out_length
	movf xbee_out_length, W
	bz txif_end_transmission
	; Check pointer value.
	movf xbee_out_ptr, W
	DISPATCH_INIT
	DISPATCH_COND 0xFF, txif_send_sop
	DISPATCH_COND 0xFE, txif_send_length_msb
	DISPATCH_COND 0xFD, txif_send_length_lsb
	DISPATCH_END_RESTORE
	xorwf xbee_out_length, W
	bz txif_send_checksum
	; We're sending normal payload data.
	; Address the byte to send.
	lfsr 0, xbee_out_apiid
	movf xbee_out_ptr, W
	addwf FSR0L, F
	; Check if we've already escaped this byte.
	movf INDF0, W
	tstfsz xbee_out_escape
	bra txif_main_escaped
	DISPATCH_INIT
	DISPATCH_COND 0x7E, txif_escape
	DISPATCH_COND 0x7D, txif_escape
	DISPATCH_COND 0x11, txif_escape
	DISPATCH_COND 0x13, txif_escape
	DISPATCH_END_RESTORE
	; Escaping is not needed. Send the byte.
	movwf TXREG
	incf xbee_out_ptr, F
	clrf xbee_out_escape
	return

txif_main_escaped:
	; The escape was sent for this byte. Send the adjusted value.
	xorlw 0x20
	movwf TXREG
	; Move to the next byte.
	incf xbee_out_ptr, F
	clrf xbee_out_escape
	return

txif_send_sop:
	; Send a start-of-packet byte.
	movlw 0x7E
	movwf TXREG
	movlw 0xFE
	movwf xbee_out_ptr
	clrf xbee_out_escape
	return

txif_send_length_msb:
	; Send a zero (we don't do extra-long packets).
	clrf TXREG
	movlw 0xFD
	movwf xbee_out_ptr
	return

txif_send_length_lsb:
	; Check if we've already escaped this byte.
	tstfsz xbee_out_escape
	bra txif_send_length_lsb_escaped
	movf xbee_out_length, W
	DISPATCH_INIT
	DISPATCH_COND 0x7E, txif_escape
	DISPATCH_COND 0x7D, txif_escape
	DISPATCH_COND 0x11, txif_escape
	DISPATCH_COND 0x13, txif_escape
	DISPATCH_END_RESTORE
	; Escaping is not needed. Send the byte.
	movwf TXREG
	clrf xbee_out_ptr
	clrf xbee_out_escape
	return

txif_send_length_lsb_escaped:
	; The escape was sent for this byte. Send the adjusted value.
	xorlw 0x20
	movwf TXREG
	; Move to the first data byte.
	clrf xbee_out_ptr
	clrf xbee_out_escape
	return

txif_send_checksum:
	; Check if the escape flag is set. That would mean we've already calculated
	; the checksum and stored it in xbee_out_apiid, but we need to send the
	; adjusted checksum byte.
	tstfsz xbee_out_escape
	bra txif_send_checksum_escaped
	; The checksum isn't calculated yet. Calculate it. Use xbee_out_escape as
	; a temporary loop counter. Accumulate the checksum in WREG.
	movf xbee_out_length, W
	movwf xbee_out_escape
	; Now go through the data bytes.
	lfsr 0, xbee_out_apiid
	movlw 0
txif_send_checksum_calculate:
	addwf POSTINC0, W
	decf xbee_out_escape
	bnz txif_send_checksum_calculate
	; Subtract the sum from 0xFF.
	negf WREG
	addlw 0xFF
	; Checksum is now calculated. Save it in xbee_out_apiid.
	movwf xbee_out_apiid
	; Check if it needs escaping. If it does, we'll send the escape code now and
	; come back here later (since xbee_out_ptr and xbee_out_length are
	; unchanged). We'll then see that xbee_out_escape is true, and just send the
	; precalculated checksum (from xbee_out_apiid) after adjusting for escaping
	; rules.
	DISPATCH_INIT
	DISPATCH_COND 0x7E, txif_escape
	DISPATCH_COND 0x7D, txif_escape
	DISPATCH_COND 0x11, txif_escape
	DISPATCH_COND 0x13, txif_escape
	DISPATCH_END_RESTORE
	; We don't need to escape this checksum byte. Just send it straight out.
	movwf TXREG
	; We're done!
	bra txif_end_transmission

txif_send_checksum_escaped:
	; Send the adjusted byte.
	movf xbee_out_apiid, W
	xorlw 0x20
	movwf TXREG
	; We're done!
	bra txif_end_transmission

txif_escape:
	; Send the escape code.
	movlw 0x7D
	movwf TXREG
	; Flag that it has been sent.
	setf xbee_out_escape
	return

txif_end_transmission:
	; Clear the length byte so we don't think there's any data to send.
	clrf xbee_out_length
	; Set the pointer so that the next thing we'll do (once we get new data) is
	; send the SOP.
	setf xbee_out_ptr
	; Disable the interrupt enable flag so we won't get invoked until someone
	; reenables the flag when they queue up more data to send.
	bcf PIE1, TXIE
	return



	; Main code.
bootload:
	; Initialize XBee buffers to empty.
	banksel xbee_in_ptr
	setf xbee_in_ptr
	banksel xbee_out_ptr
	setf xbee_out_ptr

	; Initialize IRPs to empty.
	banksel irp0
	clrf irp0 + 0
	clrf irp1 + 0
	clrf irp2 + 0
	clrf irp3 + 0

	; Initialize the IRP pointer.
	banksel irpptr
	clrf irpptr

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

	; Start up the USART and enable interrupts.
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
	bcf IPR1, RCIP
	bcf IPR1, TXIP
	bsf PIE1, RCIE
	bcf PIE1, TXIE

irp_loop:
	; Check if the IO pin has gone low.
	btfss PORT_XBEE_BL, PIN_XBEE_BL
	reset

	; Address the IRP.
	rcall irpptr_to_irp_fsr2

	; Check IRP status.
	; NOTE: Concurrency issue!
	; The ISR might occur at any time and write to the IRP stat+op byte.
	; However, the MOVF is atomic with respect to the ISR. Therefore, we will
	; see a consistent (albeit potentially old) value for the IRP status nybble.
	;
	; Thus, it is possible that either:
	; (1) The status changes from IRP_STATUS_PENDING to something else after the
	; MOVF. This is fine because the status nybble is rechecked by handle_irp.
	; (2) The status changes from something else to IRP_STATUS_PENDING after the
	; MOVF. This is fine because this IRP will be ignored and then noticed on a
	; future iteration.
	movf [0], W
	andlw 0xF0
	xorlw IRP_STATUS_PENDING << 4
	skpnz
	rcall handle_irp

	; Move on to the next IRP.
	banksel irpptr
	incf irpptr, F
	movlw 0x03
	andwf irpptr, F
	bra irp_loop



handle_irp:
	; Mark the IRP as busy.
	; NOTE: Concurrency issue!
	; The IRP *was* marked IRP_STATUS_PENDING in the main loop before handle_irp
	; was called. It's possible that the ISR has executed between then and now,
	; which may have resulted in the IRP being cleared. Therefore, we need to
	; double-check, in an interrupts-disabled block, whether the IRP is still
	; pending and, if it is, mark it as busy.
	bcf INTCON, GIE
	movf [0], W
	andlw 0xF0
	xorlw IRP_STATUS_PENDING << 4
	bz handle_irp_lock
	bsf INTCON, GIE
	return
handle_irp_lock:
	movf [0], W
	andlw 0x0F
	iorlw IRP_STATUS_BUSY << 4
	movwf [0]
	bsf INTCON, GIE

	; The rest of the handler does not have any concurrency issues, because the
	; ISR never modifies a busy IRP.

	; Dispatch the IRP based on what type of IO operation was requested.
	andlw 0x0F
	DISPATCH_INIT
	DISPATCH_COND IO_OPERATION_ERASE_BLOCK, handle_erase_block
	DISPATCH_COND IO_OPERATION_WRITE_PAGE, handle_write_page
	DISPATCH_COND IO_OPERATION_CRC_SECTOR, handle_crc_sector
	DISPATCH_END_NORESTORE

	; Illegal IO operation. Flag the error.
	; NOTE: Concurrency issue!
	; The IRP is currently marked as BUSY. The serial receive ISR will never
	; write to a busy IRP. The stat+op byte is only written to by one single
	; MOVWF instruction, which is atomic with respect to the ISR.
	movf [0], W
	andlw 0x0F
	iorlw IRP_STATUS_BAD_OPERATION << 4
	movwf [0]
	return



handle_erase_block:
	; Check that the page number is a multiple of 256.
	tstfsz [2]
	bra irp_set_status_bad_address

	; Check that the page number is within the size of the Flash.
	movlw 33
	cpfslt [1]
	bra irp_set_status_bad_address

	; Send the WRITE ENABLE command.
	rcall send_write_enable

	; Send the BLOCK ERASE command (0xD8) with address.
	rcall select_chip
	movlw 0xD8
	call spi_send
	movf [1], W
	call spi_send
	movlw 0
	call spi_send
	movlw 0
	call spi_send
	rcall deselect_chip

	; Poll the status word and complete the IRP when done.
	bra poll_status_and_mark_irp



handle_write_page:
	; Check that the page number is within the size of the Flash.
	movlw 33
	cpfslt [1]
	bra irp_set_status_bad_address

	; Send the WRITE ENABLE command.
	rcall send_write_enable

	; Send the PAGE PROGRAM command (0x02) with address.
	rcall select_chip
	movlw 0x02
	call spi_send
	movf [1], W
	call spi_send
	movf [2], W
	call spi_send
	movlw 0
	call spi_send

	; Address the buffer.
	rcall irpptr_to_buffer_fsr2

	; Send the data.
	banksel bytecounter
	clrf bytecounter
handle_write_page_loop:
	movf POSTINC2, W
	call spi_send
	decf bytecounter, F
	bnz handle_write_page_loop
	rcall deselect_chip

	; Address the IRP again.
	rcall irpptr_to_irp_fsr2

	; Poll the status word and complete the IRP when done.
	bra poll_status_and_mark_irp



handle_crc_sector:
	; Check that the page number is a multiple of 16.
	movf [2], W
	andlw 0x0F
	bnz irp_set_status_bad_address

	; Check that the page number is within the size of the Flash.
	movlw 33
	cpfslt [1]
	bra irp_set_status_bad_address

	; Send the READ DATA command (0x03) with address.
	rcall select_chip
	movlw 0x03
	call spi_send
	movf [1], W
	call spi_send
	movf [2], W
	call spi_send
	movlw 0
	call spi_send

	; Address the buffer.
	rcall irpptr_to_buffer_fsr2

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
	decf bytecounter, F
	bnz handle_crc_sector_byteloop

	; A page is finished. Advance the FSR to the next CRC position.
	addfsr 2, 2

	; Decrement page count and loop if nonzero.
	decf pagecounter, F
	bnz handle_crc_sector_pageloop

	; Address the IRP.
	rcall irpptr_to_irp_fsr2

	; Mark IRP complete.
	movf [0], W
	andlw 0x0F
	iorlw IRP_STATUS_COMPLETE << 4
	movwf [0]
	return



irp_set_status_bad_address:
	movf [0], W
	andlw 0x0F
	iorlw IRP_STATUS_BAD_ADDRESS << 4
	movwf [0]
	return



poll_status_and_mark_irp:
	; Poll the STATUS byte in the Flash chip until the chip is no longer busy.
	rcall select_chip
	movlw 0x05
	call spi_send
poll_status_and_mark_irp_loop:
	call spi_receive
	btfsc WREG, 0
	bra poll_status_and_mark_irp_loop
	rcall deselect_chip

	; Mark the IRP completed.
	movf [0], W
	andlw 0x0F
	iorlw IRP_STATUS_COMPLETE << 4
	movwf [0]
	return



irpptr_to_irp_fsr2:
	banksel irpptr
	movf irpptr, W
	lfsr 2, irp0
	addwf FSR2L, F
	addwf FSR2L, F
	addwf FSR2L, F
	return



irpptr_to_buffer_fsr2:
	banksel irpptr
	movf irpptr, W
	DISPATCH_INIT
	DISPATCH_COND 0, irpptr_to_buffer_fsr2_0
	DISPATCH_COND 1, irpptr_to_buffer_fsr2_1
	DISPATCH_COND 2, irpptr_to_buffer_fsr2_2
	DISPATCH_COND 3, irpptr_to_buffer_fsr2_3
	DISPATCH_END_NORESTORE
irpptr_to_buffer_fsr2_0:
	lfsr 2, buf0
	return
irpptr_to_buffer_fsr2_1:
	lfsr 2, buf1
	return
irpptr_to_buffer_fsr2_2:
	lfsr 2, buf2
	return
irpptr_to_buffer_fsr2_3:
	lfsr 2, buf3
	return



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

	end
