	;
	; PIN ASSIGNMENTS:
	;
	;           RA0 - Counter 0 CP In
	;           RA1 - Counter 0 UD In
	;           RA2 - Counter 1 CP In
	;           RA3 - Counter 1 UD In
	;           RA4 - Counter 2 CP In
	;           RA5 - Counter 2 UD In
	;           RA6 - Crystal
	;           RB0 - Count Out 0
	;           RB1 - Count Out 1
	;           RB2 - Count Out 2
	;           RB3 - Count Out 3
	;           RB4 - Count Out 4
	;           RB5 - Count Out 5
	;           RB6 - Count Out 6
	;           RB7 - Count Out 7
	;           RC0 - No Connection
	;           RC1 - Counter 3 CP In
	;           RC2 - Counter 3 UD In
	; [in only] RC4 - Counter Select 0 In
	; [in only] RC5 - Counter Select 1 In
	;           RC6 - Counter Select 2 In
	;           RC7 - Mode (see below)
	;
	;
	; OPERATION:
	;
	;   1. Set SELECT[2:1] to the index of the counter to read.
	;   2. Set SELECT0 low.
	;   3. Set MODE high.
	;   4. Wait at least 6 microseconds.
	;   5. Set MODE low.
	;   6. Wait at least 6 microseconds.
	;   7. Read the low byte of the counter from COUNT OUT.
	;   8. Set SELECT0 high.
	;   9. Wait at least 6 microseconds.
	;  10. Read the high byte of the counter from COUNT OUT.
	;  11. Repeat from #1 for another counter.
	;
	;  Through the MODE pin, this sequence guarantees that the 16-bit
	;  count value will be delivered atomically: the value delivered
	;  is precisely the value of the counter at the instant when MODE
	;  was driven low, even if the count changes later in the sequence.
	;  This prevents byte-tearing across the value.
	;

	radix dec
	processor 18F2550
#include <p18f2550.inc>

	__config _CONFIG1L, _PLLDIV_5_1L & _CPUDIV_OSC1_PLL2_1L & _USBDIV_1_1L
	__config _CONFIG1H, _FOSC_HSPLL_HS_1H & _FCMEM_OFF_1H & _IESO_OFF_1H
	__config _CONFIG2L, _PWRT_ON_2L & _BOR_OFF_2L & _BORV_0_2L & _VREGEN_OFF_2L
	__config _CONFIG2H, _WDT_OFF_2H & _WDTPS_1_2H
	__config _CONFIG3H, _MCLRE_ON_3H & _LPT1OSC_ON_3H & _PBADEN_OFF_3H & _CCP2MX_OFF_3H
	__config _CONFIG4L, _STVREN_ON_4L & _LVP_OFF_4L & _XINST_OFF_4L & _DEBUG_OFF_4L
	__config _CONFIG5L, _CP0_OFF_5L & _CP1_OFF_5L & _CP2_OFF_5L & _CP3_OFF_5L
	__config _CONFIG5H, _CPB_OFF_5H & _CPD_OFF_5H
	__config _CONFIG6L, _WRT0_OFF_6L & _WRT1_OFF_6L & _WRT2_OFF_6L & _WRT3_OFF_6L
	__config _CONFIG6H, _WRTB_OFF_6H & _WRTC_OFF_6H & _WRTD_OFF_6H
	__config _CONFIG7L, _EBTR0_OFF_7L & _EBTR1_OFF_7L & _EBTR2_OFF_7L & _EBTR3_OFF_7L
	__config _CONFIG7H, _EBTRB_OFF_7H



	udata_acs
ramstart:

prevstates:
	res 4
counts:
	res 8
shadows:
	res 2
prevstate_lut:
	res 32
delta_lut:
	res 32
temp:
	res 1

ramend:



LUTs code_pack
;
; This LUT is indexed by (PREVIOUS_STATE * 4 + NEW_STATE) * 2.
; The value should be (NEW_STATE * 4) * 2.
;
prevstate_lut_data:
	db 0, -1, 8, -1, 16, -1, 24, -1
	db 0, -1, 8, -1, 16, -1, 24, -1
	db 0, -1, 8, -1, 16, -1, 24, -1
	db 0, -1, 8, -1, 16, -1, 24, -1

;
; This LUT is indexed by (PREVIOUS_STATE * 4 + NEW_STATE) * 2 byte offset.
; The value should be +1, 0, or -1, the amount to change the count by.
; Values are 16 bits, high-order first.
;
DLE macro delta
	db HIGH(delta), LOW(delta)
	endm

delta_lut_data:
errorlevel -202 ; Argument out of range. Least significant bits used.
;      /---- NEW ----\
; UD:  |  0   |  1   |       UD:  \
; CP:  |0 | 1 | 0 | 1|  CP:       |
	        ; [FROM UD, FROM CP] [TO UD, TO CP]
	DLE( 0) ; [   0        0   ] [  0      0  ]
	DLE(-1) ; [   0        0   ] [  0      1  ]
	DLE( 0) ; [   0        0   ] [  1      0  ]
	DLE( 0) ; [   0        0   ] [  1      1  ]
	DLE( 1) ; [   0        1   ] [  0      0  ]
	DLE( 0) ; [   0        1   ] [  0      1  ]
	DLE( 0) ; [   0        1   ] [  1      0  ]
	DLE( 0) ; [   0        1   ] [  1      1  ]
	DLE( 0) ; [   1        0   ] [  0      0  ]
	DLE( 0) ; [   1        0   ] [  0      1  ]
	DLE( 0) ; [   1        0   ] [  1      0  ]
	DLE( 1) ; [   1        0   ] [  1      1  ]
	DLE( 0) ; [   1        1   ] [  0      0  ]
	DLE( 0) ; [   1        1   ] [  0      1  ]
	DLE(-1) ; [   1        1   ] [  1      0  ]
	DLE( 0) ; [   1        1   ] [  1      1  ]
errorlevel +202



STARTUP code
	goto entry



	; Loads TBLPTR with an address. Destroys WREG.
LOAD_TBLPTR macro ADDRESS
	movlw UPPER(ADDRESS)
	movwf TBLPTRU
	movlw HIGH(ADDRESS)
	movwf TBLPTRH
	movlw LOW(ADDRESS)
	movwf TBLPTRL
	endm



	; Expects:
	; WREG = new value (possibly with higher bits set)
	; FSR1 -> prevstate_lut
	; FSR2 -> delta_lut
PROCESS_COUNTER macro COUNTER_INDEX
	; Clear higher bits.
	andlw 0x06                                             ; 1 cycle
	; Bring in (previous state * 4) * 2.
	iorwf prevstates + COUNTER_INDEX, W                    ; 1 cycle
	; Copy the value from prevstate_lut to prevstates.
	movff PLUSW1, prevstates + COUNTER_INDEX               ; 2 cycles
	; Copy the high byte from delta_lut to temp.
	movff PLUSW2, temp                                     ; 2 cycles
	; Load the low byte from delta_lut to WREG.
	addlw 1                                                ; 1 cycle
	movf PLUSW2, W                                         ; 1 cycle
	; Add this value to the count.
	addwf counts + COUNTER_INDEX, F                        ; 1 cycle
	movf temp, W                                           ; 1 cycle
	addwfc counts + COUNTER_INDEX + 1, F                   ; 1 cycle
	endm
	                                                       ; ==========
	                                                       ; 11 cycles



	code
entry:
	; Kill unwanted peripherals.
	clrf T0CON
	movlw (1 << PCFG3) | (1 << PCFG2) | (1 << PCFG1) | (1 << PCFG0)
	movwf ADCON1
	movlw (1 << UTRDIS)
	movwf UCFG

	; Set up ports.
	clrf LATB
	clrf TRISB

	; Clear RAM.
	lfsr 0, ramstart
	movlw ramend - ramstart
clearram:
	clrf POSTINC0
	addlw -1
	bnz clearram

	; Load LUTs.
	lfsr 0, prevstate_lut
	LOAD_TBLPTR prevstate_lut_data
	call load_lut

	lfsr 0, delta_lut
	LOAD_TBLPTR delta_lut_data
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
	rlncf PORTA, W                ; 1 cycle
	PROCESS_COUNTER 0             ; 11 cycles

	rrncf PORTA, W                ; 1 cycle
	PROCESS_COUNTER 1             ; 11 cycles

	swapf PORTA, W                ; 1 cycle
	PROCESS_COUNTER 2             ; 11 cycles

	movf PORTC, W                 ; 1 cycle
	PROCESS_COUNTER 3             ; 11 cycles

	btfss PORTC, 7                ; 1 cycle
	                              ; =========
	                              ; 49 cycles
	
	                              ; BRANCH NOT TAKEN CASE:
	bra latched                   ; 1 cycle
	swapf PORTC, W                ; 1 cycle
	andlw 0x06                    ; 1 cycle
	movff PLUSW0, shadows+0       ; 2 cycles
	addlw 1                       ; 1 cycle
	movff PLUSW0, shadows+1       ; 2 cycles
	bra loop                      ; 2 cycles
	                              ; ========
	                              ; 10 cycles + 49 cycles = 59 cycles

latched:                          ; BRANCH TAKEN CASE:
	                              ; 2 cycles from the jump
	movf shadows, W               ; 1 cycle
	btfsc PORTC, 4                ; 1 cycle
	movf shadows+1, W             ; 1 cycle
	movwf LATB                    ; 1 cycle
	nop                           ; 1 cycle
	nop                           ; 1 cycle
	bra loop                      ; 2 cycles
	                              ; ========
	                              ; 10 cycles + 49 cycles = 59 cycles

	;
	; In both cases, a full loop iteration takes 59 cycles.
	; This means that running from the PLL, the system clock
	; is 48MHz and we get 12MIPS.
	;
	; Then 12000000 / 59 = 203389.83 loop iterations per second,
	; 203389.83 / (360*4) = 141.24294 wheel rotations per second,
	; 141.24294 * 60 = 8474.5763 RPM
	;
	; So we can successfully measure just over 8000 RPM.
	;
	; Furthermore:
	;
	; 1000000 / 203389.83 = 4.917us
	;
	; So a loop iteration takes a little under 5us to complete.
	;



	; Loads a 32-byte LUT from Flash at TBLPTR to RAM at FSR0.
load_lut:
	movlw 32
load_lut_loop:
	tblrd *+
	movff TABLAT, POSTINC0
	addlw -1
	bnz load_lut_loop
	return

	end

