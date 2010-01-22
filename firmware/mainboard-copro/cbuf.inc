	; asmsyntax=pic

	; Circular Buffer
	; ===============
	;
	; The macros here allow a circular buffer to be allocated and manipulated.



	; Macro CBUF_DECLARE
	; ==================
	;
	; Declares a data block for a circular buffer with 16 bytes of payload
	; storage.
	;
CBUF_DECLARE macro buffer
buffer: res 19
	endm



	; Macro CBUF_INIT
	; ===============
	;
	; Initializes a circular buffer to be empty. Destroys WREG.
	;
CBUF_INIT macro buffer
	; Temp byte need not be initialized.
	; Index of last byte read is zero.
	clrf buffer + 1
	; Index of next byte to write is one.
	movlw 1
	movwf buffer + 2
	endm



	; Macro CBUF_LEVEL
	; ================
	;
	; Returns the number of bytes stored in the buffer into WREG. If called
	; concurrently with CBUF_PUT or CBUF_GET, returns the correct value before
	; or after the operation, but not anything else.
	;
CBUF_LEVEL macro buffer
	; Number of bytes stored = next byte to write - last byte read - 1.
	; Number of bytes stored is always between 0 and 15.
	movf buffer + 1, W
	addlw 1
	subwf buffer + 2, W
	andlw 15
	endm



	; Macro CBUF_PUT
	; ==============
	;
	; Stores the contents of WREG into the buffer whose name is "buffer". In the
	; process, destroys FSR<fsrn> of your choice and WREG. Cannot be called
	; concurrently with other CBUF_PUTs, but *can* be called concurrently with a
	; CBUF_GET.
	;
CBUF_PUT macro buffer, fsrn
	; Store WREG into temp byte.
	movwf buffer + 0
	; Load index of next byte to write.
	movf buffer + 2, W
	; Initialize FSR.
	lfsr fsrn, buffer + 3
	; Store byte to target.
	movff buffer + 0, PLUSW#v(fsrn)
	; Increment write pointer.
	addlw 1
	andlw 15
	movwf buffer + 2
	endm



	; Macro CBUF_GET
	; ==============
	;
	; Reads a byte from "buffer" into WREG. In the process, destroys FSR<fsrn>
	; of your choice. Cannot be called concurrently with other CBUF_GETs, but
	; *can* be called concurrently with a CBUF_PUT.
	;
CBUF_GET macro buffer, fsrn
	LOCAL wait_empty
	; Load index of last byte read.
	movf buffer + 1, W
	; Increment read pointer.
	addlw 1
	andlw 15
	movwf buffer + 1
	; Wait until buffer is not empty (buffer is empty when new read pointer
	; equals write pointer)
wait_empty:
	movf buffer + 1, W
	xorwf buffer + 2, W
	bz wait_empty
	; Read byte.
	movf buffer + 1, W
	lfsr fsrn, buffer + 3
	movf PLUSW#v(fsrn), W
	endm
