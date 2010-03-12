	; asmsyntax=pic

	; bootblock.asm
	; =============
	;
	; This file contains address zero, where the PIC resets to, the physical
	; interrupt vectors at 0x8 and 0x18, and code to copy a new application
	; image from the staging area to the execution area when instructed to do so
	; as the completion phase of a wireless firmware upgrade.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#include "pins.inc"



	udata
buffer: res 32



bootentry code 0x0
	; Jump to a more useful (larger) area of ROM.
	goto bootup

bootinth code 0x8
	; Reflect to the application.
	goto 0x808

bootintl code 0x18
	; Reflect to the application.
	goto 0x818

bootblock code
bootup:
	; Check the EEPROM flag. To execute an upgrade, the first two bytes of
	; EEPROM contain 0x1234.
	clrf EEADR
	clrf EECON1
	bsf EECON1, RD
	movf EEDATA, W
	xorlw 0x12
	bnz bootup_to_main
	incf EEADR, F
	bsf EECON1, RD
	movf EEDATA, W
	xorlw 0x34
	bnz bootup_to_main



	; We need to execute an upgrade.
	; Turn on the LED.
	bcf TRIS_LED, PIN_LED
	bsf LAT_LED, PIN_LED

	; First, erase everything from 0x800 to 0x3FFF.
	movlw LOW(0x800)
	movwf TBLPTRL
	movlw HIGH(0x800)
	movwf TBLPTRH
	movlw UPPER(0x800)
	movwf TBLPTRU
erase_loop:
	movlw (1 << EEPGD) | (1 << FREE) | (1 << WREN)
	movwf EECON1
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
	clrf EECON1
	movlw 64
	addwf TBLPTRL, F
	skpnc
	incf TBLPTRH, F
	movlw 0x3F
	cpfsgt TBLPTRH
	bra erase_loop

	; Now copy data from 0x4800 through 0x7FFF to 0x800 through 0x3FFF.
	movlw LOW(0x800)
	movwf TBLPTRL
	movlw HIGH(0x800)
	movwf TBLPTRH
	movlw UPPER(0x800)
	movwf TBLPTRU
copy_outer_loop:
	; Bounce TBLPTR up to the staging area.
	bsf TBLPTRH, 6
	; Copy 32 bytes to buffer.
	lfsr 0, buffer
	movlw 32
copy_to_buffer_loop:
	tblrd *+
	movff TABLAT, POSTINC0
	decfsz WREG, W
	bra copy_to_buffer_loop

	; Shift TBLPTR back by 32 bytes.
	tblrd *-
	movlw -31
	addwf TBLPTRL, F
	; Bounce TBLPTR down to the execution area.
	bcf TBLPTRH, 6
	; Copy 32 bytes to holding registers
	lfsr 0, buffer
	movlw 32
	tblrd *-
copy_from_buffer_loop:
	movff POSTINC0, TABLAT
	tblwt +*
	decfsz WREG, W
	bra copy_from_buffer_loop
	; Write the data.
	movlw (1 << EEPGD) | (1 << WREN)
	movwf EECON1
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
	clrf EECON1
	; Finish when we're sitting at 0x4000.
	tblrd *+
	movf TBLPTRH, W
	xorlw HIGH(0x4000)
	bnz copy_outer_loop
	movf TBLPTRL, W
	xorlw LOW(0x4000)
	bnz copy_outer_loop

	; Copying is finished. We now want to clear the first two bytes of EEPROM so
	; we aren't confused by the flag still being set in future.
	clrf EEADR
	setf EEDATA
	movlw (1 << WREN)
	movwf EECON1
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
erase_eeprom1_loop:
	btfsc EECON1, WR
	bra erase_eeprom1_loop

	incf EEADR, F
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
erase_eeprom2_loop:
	btfsc EECON1, WR
	bra erase_eeprom2_loop

	; Reset the CPU.
	clrf EECON1
	reset



bootup_to_main:
	goto 0x800

	end

