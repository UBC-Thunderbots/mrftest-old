	; asmsyntax=pic

	; Dispatch Table
	; ==============
	;
	; The macros here enable a dispatch table to be built in code efficiently.



	; Macro DISPATCH_INIT
	; ===================
	;
	; Begins a dispatch table which dispatches various values of WREG to target
	; labels.
	;
DISPATCH_INIT macro
DISPATCH_LAST_VALUE = 0
	endm



	; Macro DISPATCH_COND value, target
	; =================================
	;
	; Outputs a dispatch table entry such that if WREG == value, jumps to
	; target. Target must be within range of a conditional branch (e.g. bz) of
	; the current instruction.
	;
DISPATCH_COND macro value, target
	xorlw DISPATCH_LAST_VALUE ^ (value)
	bz target
DISPATCH_LAST_VALUE = value
	endm



	; Macro DISPATCH_BRA value, target
	; ================================
	;
	; Outputs a dispatch table entry such that if WREG == value, jumps to
	; target. Target must be within range of an unconditional branch (bra) of
	; the current instruction.
	;
DISPATCH_BRA macro value, target
	xorlw DISPATCH_LAST_VALUE ^ (value)
	skpnz
	bra target
DISPATCH_LAST_VALUE = value
	endm



	; Macro DISPATCH_GOTO value, target
	; =================================
	;
	; Outputs a dispatch table entry such that if WREG == value, jumps to
	; target.
	;
DISPATCH_GOTO macro value, target
	xorlw DISPATCH_LAST_VALUE ^ (value)
	skpnz
	goto target
DISPATCH_LAST_VALUE = value
	endm



	; Macro DISPATCH_END_RESTORE
	; ==========================
	;
	; Ends a dispatch table. Restores the value of WREG.
	;
DISPATCH_END_RESTORE macro
	xorlw DISPATCH_LAST_VALUE
	endm



	; Macro DISPATCH_END_NORESTORE
	; ============================
	;
	; Ends a dispatch table. Does not restore WREG.
	;
DISPATCH_END_NORESTORE macro
	endm
