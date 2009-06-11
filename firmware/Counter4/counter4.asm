	;
	; PIN ASSIGNMENTS:
	;
	; Counters:
	; RA0:RA1 -> counter 0
	; RA4:RA5 -> counter 1
	; RC7:RC0 -> counter 2
	; RC1:RC2 -> counter 3
	; The first pin is CP (count forward on rising edge, backward on falling)
	; The second pin is UD (count up = forward if 1, down = forward if 0)
	;
	; Misc:
	; RC4:RC5 -> output select
	; RB0:RB7 -> counter output
	;

	radix dec
	processor 18F2550
#include <p18f2550.inc>



	udata_acs
prevstates:
	res 4
counts:
	res 4
prevstate_lut:
	res 16
delta_lut:
	res 16



LUTs code_pack
;
; This LUT is indexed by PREVIOUS_STATE * 4 + NEW_STATE.
; The value should be NEW_STATE * 4.
;
prevstate_lut_data:
	db 0, 4, 8, 12
	db 0, 4, 8, 12
	db 0, 4, 8, 12
	db 0, 4, 8, 12

;
; This LUT is indexed by PREVIOUS_STATE * 4 + NEW_STATE.
; The value should be +1, 0, or -1, the amount to change the count by.
; CP is low-order
; UD is high-order
;
delta_lut_data:
errorlevel -202 ; Argument out of range. Least significant bits used.
;      /---- NEW ----\
; UD:  |  0   |  1   |       UD:  \
; CP:  |0 | 1 | 0 | 1|  CP:       |
	db  0, -1,  0,  0  ; 0    0   | OLD
	db  1,  0,  0,  0  ; 1    0   |
	db  0,  0,  0,  1  ; 0    1   |
	db  0,  0, -1,  0  ; 1    1   /
errorlevel +202



STARTUP code
	goto entry



	; Expects:
	; WREG = new value (possibly with higher bits set)
	; FSR1 -> prevstate_lut
	; FSR2 -> delta_lut
PROCESS_COUNTER macro COUNTER_INDEX
	; Clear higher bits.
	andlw 0x03                                             ; 1 cycle
	; Bring in the previous state * 4.
	iorwf prevstates + COUNTER_INDEX, W                    ; 1 cycle
	; Copy the value from prevstate_lut to prevstates.
	movff PLUSW1, prevstates + COUNTER_INDEX               ; 2 cycles
	; Load the value from delta_lut to WREG.
	movf PLUSW2, W                                         ; 1 cycle
	; Add this value to the count.
	addwf counts + COUNTER_INDEX, F                        ; 1 cycle
	endm
	                                                       ; ==========
	                                                       ; 6 cycles



	code
entry:
	; Set up ports.
	clrf LATA
	clrf LATB
	clrf LATC
	movlw (1 << 5) | (1 << 4) | (1 << 1) | (1 << 5)
	movwf TRISA
	clrf TRISB
	movlw (1 << 7) | (1 << 2) | (1 << 1) | (1 << 0)
	movwf TRISC

	; Clear prevstates.
	clrf prevstates+0
	clrf prevstates+1
	clrf prevstates+2
	clrf prevstates+3

	; Clear counts.
	clrf counts+0
	clrf counts+1
	clrf counts+2
	clrf counts+3

	; Load LUTs.
	lfsr 0, prevstate_lut
	movlw UPPER(prevstate_lut_data)
	movwf TBLPTRU
	movlw HIGH(prevstate_lut_data)
	movwf TBLPTRH
	movlw LOW(prevstate_lut_data)
	movwf TBLPTRL
	call load_lut

	lfsr 0, delta_lut
	movlw UPPER(delta_lut_data)
	movwf TBLPTRU
	movlw HIGH(delta_lut_data)
	movwf TBLPTRH
	movlw LOW(delta_lut_data)
	movwf TBLPTRL
	call load_lut

	; Point FSRs at useful places:
	; FSR0 -> counts
	; FSR1 -> prevstate_lut
	; FSR2 -> delta_lut
	lfsr 0, counts
	lfsr 1, prevstate_lut
	lfsr 2, delta_lut

	; Start of main loop.
loop:
	movf PORTA, W                 ; 1 cycle
	PROCESS_COUNTER 0             ; 6 cycles

	swapf PORTA, W                ; 1 cycle
	PROCESS_COUNTER 1             ; 6 cycles

	rlncf PORTC, W                ; 1 cycle
	PROCESS_COUNTER 2             ; 6 cycles

	rrncf PORTC, W                ; 1 cycle
	PROCESS_COUNTER 3             ; 6 cycles

	swapf PORTC, W                ; 1 cycle
	andlw 0x03                    ; 1 cycle
	movff PLUSW0, LATB            ; 2 cycles
	bra loop                      ; 2 cycles
	                              ; ==========
	                              ; 34 cycles

	; Loads a 16-byte LUT from Flash at TBLPTR to RAM at FSR0.
load_lut:
	movlw 16
load_lut_loop:
	tblrd *+
	movff TABLAT, POSTINC0
	decfsz WREG, W
	bra load_lut_loop
	return

	end

