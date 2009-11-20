	; asmsyntax=pic
	radix dec
	processor 18F4550
#include <p18f4550.inc>
#define IMPL
#include "dbgprint.inc"
#undefine IMPL

TIMER3_COUNT equ 4000000/9600/1
TIMER3_FUDGE equ 0
TIMER3_RESET equ 65536 - (TIMER3_COUNT + TIMER3_FUDGE)

	if TIMER3_RESET < 0
		error "Timer count out of range"
	endif

BUFFER_SIZE equ 100





	udata
INTCON_save: res 1
curbyte: res 1
bitsleft: res 1
buffer_used: res 1
buffer: res BUFFER_SIZE * 2
hextemp: res 1





	code
nl:	db 13,10,0
hexdigits:
	db '0',0
	db '1',0
	db '2',0
	db '3',0
	db '4',0
	db '5',0
	db '6',0
	db '7',0
	db '8',0
	db '9',0
	db 'A',0
	db 'B',0
	db 'C',0
	db 'D',0
	db 'E',0
	db 'F',0





dbgprint_init:
	; Enable RB pullups so that RB6 doesn't float when PICkit2 not attached.
	bcf INTCON2, NOT_RBPU

	; Set the serial port to output mode and idle the line.
	bsf LATB, 7
	bcf TRISB, 7

	; Configure timer 3 to tick 1200 times per second.
	movlw HIGH(TIMER3_RESET)
	movwf TMR3H
	movlw LOW(TIMER3_RESET)
	movwf TMR3L
	movlw (1 << TMR3ON)
	movwf T3CON
	bcf PIR2, TMR3IF
	bsf PIE2, TMR3IE

	; Send 10 1-bits (8 1-data-bits plus a 2 stop bits) to stabilize the line.
	banksel curbyte
	movlw 0xFF
	movwf curbyte
	movlw 10
	movwf bitsleft

	; Clear the buffer counter.
	clrf buffer_used

	return





dbgprint_rom:
	; Read the first byte - don't enqueue an empty string.
	tblrd *
	movf TABLAT, F
	btfsc STATUS, Z
	return

	banksel buffer_used

	; Disable interrupts.
	movf INTCON, W
	bcf INTCON, GIEH
	bcf INTCON, GIEL
	movwf INTCON_save

	; Check for sufficient buffer space.
	movlw BUFFER_SIZE
	xorwf buffer_used, W
	bz dbgprint_rom_unmask_and_return

	; Decrement TBLPTR to point one-before the string data, because we always use
	; a pre-increment read to load the data in the interrupt handler.
	tblrd *-

	; Address the proper element of the buffer.
	lfsr 0, buffer
	movf buffer_used, W
	addwf FSR0L, F
	addwf FSR0L, F

	; Store the new data.
	movff TBLPTRL, POSTINC0
	movff TBLPTRH, INDF0

	; Increment the buffer counter.
	incf buffer_used, F

	; Restore the proper value of TBLPTR.
	tblrd *+

dbgprint_rom_unmask_and_return:
	; Re-enable interrupts.
	movf INTCON_save, W
	andlw (1 << GIEH) | (1 << GIEL)
	iorwf INTCON, F

	return





dbgprint_hex:
	banksel hextemp
	movwf hextemp
	swapf WREG, W
	andlw 0x0F
	rcall dbgprint_hex_digit
	banksel hextemp
	movf hextemp, W
	andlw 0x0F
	bra dbgprint_hex_digit


dbgprint_hex_digit:
	rlncf WREG, W
	movwf TBLPTRL
	movlw LOW(hexdigits)
	addwf TBLPTRL, F
	clrf TBLPTRH
	movlw HIGH(hexdigits)
	addwfc TBLPTRH, F
	bra dbgprint_rom





dbgprint_nl:
	movlw LOW(nl)
	movwf TBLPTRL
	movlw HIGH(nl)
	movwf TBLPTRH
	rcall dbgprint_rom





dbgprint_flush:
	banksel buffer_used
dbgprint_flush_loop:
	tstfsz buffer_used
	bra dbgprint_flush_loop
	return





timer3_int:
	; Clear interrupt flag.
	bcf PIR2, TMR3IF

	; Bring the timer back to the reset value.
	movlw LOW(TIMER3_RESET)
	addwf TMR3L, F
	movlw HIGH(TIMER3_RESET)
	addwfc TMR3H, F

	; Check the number of bits left in the current byte.
	banksel curbyte
	movf bitsleft, W

	; Zero bits left (proceed to next byte).
	bz timer3_int_nobits

	; One bit left (send the stop bit).
	addlw -1
	bz timer3_int_stopbit

	; Two bits left (send the stop bit).
	addlw +1-2
	bz timer3_int_stopbit

	; Eleven bits left (send the start bit).
	addlw +2-11
	bz timer3_int_startbit

	; Some other number left (send a data bit).
	decf bitsleft, F
	rrcf curbyte, F
	btfss STATUS, C
	bcf LATB, 7
	btfsc STATUS, C
	bsf LATB, 7
	return


timer3_int_startbit:
	; Set the line low for a start bit.
	decf bitsleft, F
	nop
	nop
	bcf LATB, 7
	return


timer3_int_stopbit:
	; Set the line high for a stop bit.
	decf bitsleft, F
	nop
	nop
	bsf LATB, 7
	return


timer3_int_nobits:
	; There are no bits left in the current byte. Check if we have any strings.
	movf buffer_used, W
	btfsc STATUS, Z
	return

	; We have strings. Get the current head of the buffer.
	movff buffer, TBLPTRL
	movff buffer+1, TBLPTRH

	; Acquire the next byte of the current string.
	tblrd +*

	; Check whether it's NUL.
	movf TABLAT, W
	bz timer3_int_nobytes

	; We've got a new byte to send over the serial port.
	movwf curbyte
	movlw 11
	movwf bitsleft

	; Save the new table pointer into the transmit queue.
	movff TBLPTRL, buffer
	movff TBLPTRH, buffer+1

	; Done.
	return


timer3_int_nobytes:
	; The buffer is nonempty, but the head points to the end of a string.
	; Decrement the usage counter.
	decf buffer_used, F

	; Check if there are any elements left.
	btfsc STATUS, Z
	return

	; Copy down the data.
I=0
	while I < (BUFFER_SIZE-1)*2
		movff buffer+(2+I), buffer+I
I=I+1
	endw

	; Grab the first byte of the new string. Note: TBLPTR/TABLAT already saved.
	movff buffer, TBLPTRL
	movff buffer+1, TBLPTRH
	tblrd +*
	movff TBLPTRL, buffer
	movff TBLPTRH, buffer+1
	movff TABLAT, curbyte
	movlw 11
	movwf bitsleft

	; Done.
	return

	end
