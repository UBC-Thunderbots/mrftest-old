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



	; We need to execute an upgrade. First, erase everything from 0x800 to
	; 0x3FFF.
	movlw LOW(0x800)
	movwf TBLPTRL
	movlw HIGH(0x800)
	movwf TBLPTRH
	movlw UPPER(0x800)
	movwf TBLPTRU
	movlw (1 << EEPGD) | (1 << FREE) | (1 << WREN)
erase_loop:
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
	movlw 64
	addwf TBLPTRL, F
	movlw 0
	addwfc TBLPTRH, F
	addwfc TBLPTRU, F
	movlw 0x3F
	cpfsgt TBLPTRH
	bra erase_loop

	; Now copy data from 0x4800 through 0x7FFF to 0x800 through 0x3FFF. Here's a
	; sneaky idea: to write a block of 32 bytes, you first use TBLWT to load the
	; 32 holding registers, then set EECON1.WR to write the holding registers to
	; Flash. The lower 5 bits of TBLPTR determine which holding register is
	; written to with a particular TBLWT; those same bits are ignored during the
	; actual write. Thus, there is no requirement that TBLPTR actually point at
	; the address being written when the TBLWT is executed, only that it point
	; at an address whose lower 5 bits are the same... namely, it could point at
	; the *mirror* of the address in the staging area! So what we do is we do a
	; sequence of matched TBLRDs and TBLWTs with TBLPTR pointing in the staging
	; area to pull data from the staging area into the holding registers. Once
	; 32 bytes have been pulled, we then bounce TBLPTR down into the execution
	; area, burn the block, and bounce TBLPTR back up into the staging area
	; ready to do the next block.
	;
	; Because the execution is 0x0800 to 0x3FFF and the staging area is 0x4800
	; to 0x7FFF, the only difference between their addresses is bit 14 (aka bit
	; 6 of TBLPTRH).
	;
	; Each block write will finish with TBLPTR pointing at the last byte of the
	; block. Incrementing TBLPTR will then point at the first byte of the next
	; block. To accomodate this for the first block, start with TBLPTR pointing
	; one byte before the first block.
	movlw LOW(0x7FF)
	movwf TBLPTRL
	movlw HIGH(0x7FF)
	movwf TBLPTRH
	movlw UPPER(0x7FF)
	movwf TBLPTRU
	movlw (1 << EEPGD) | (1 << WREN)
copy_outer_loop:
	; Bounce TBLPTR up to the staging area.
	bsf TBLPTRH, 6
	; Copy 32 bytes to holding registers.
	movlw 32
copy_inner_loop:
	tblrd +*
	tblwt *
	decfsz WREG, W
	bra copy_inner_loop
	; Bounce TBLPTR down to the execution area.
	bcf TBLPTRH, 6
	; Write the data.
	movlw 0x55
	movwf EECON2
	movlw 0xAA
	movwf EECON2
	bsf EECON1, WR
	; Finish when we're sitting at 0x3FFF.
	movf TBLPTRL, W
	xorlw 0x3F
	bnz copy_outer_loop
	movf TBLPTRH, W
	xorlw 0xFF
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

