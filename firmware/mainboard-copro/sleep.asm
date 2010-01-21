	; asmsyntax=pic

	; sleep.asm
	; =========
	;
	; The functions in this file are the implementations of the functions
	; declared in sleep.inc.
	;

	radix dec
	processor 18F4550
#include <p18f4550.inc>
#define IMPL
#include "sleep.inc"
#undefine IMPL



	code
	; Clock rate is 16MHz.
	; There are 4M instruction cycles per second.
	; There are 4 instruction cycles per microsecond.
	; CALL and RCALL take 2 instruction cycles.
	; RETURN takes 2 instruction cycles.
	; Therefore, each label must consume exactly 2 instruction cycles fewer than
	; its name implies (the other 2 are consumed by the original CALL/RCALL).

sleep_100ms: ; This label wants 4*100000-2=399998 cycles.
	; 9*40000=360000.
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	; Fallthrough gives another 39998 for 399998.

sleep_10ms: ; This label wants 4*10000-2=39998 cycles.
	; 9*4000=36000.
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	; Fallthrough gives another 3998 for 39998.

sleep_1ms: ; This label wants 4*1000-2=3998 cycles.
	; 9*400=3600.
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	; Fallthrough gives another 398 for 3998.

sleep_100us: ; This label wants 4*100-2=398 cycles.
	; 9*40=360.
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	; Fallthrough gives another 38 for 398.

sleep_10us: ; This label wants 4*10-2=38 cycles.
	; 9*4=36.
	rcall sleep_1us
	rcall sleep_1us
	rcall sleep_1us
	rcall sleep_1us
	rcall sleep_1us
	rcall sleep_1us
	rcall sleep_1us
	rcall sleep_1us
	rcall sleep_1us
	; Fallthrough gives another 2 for 38.

sleep_1us: ; This label wants 4*1-2=2 cycles.
	; 2.
	return

	end
