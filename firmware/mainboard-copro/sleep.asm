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
	; Clock rate is 8MHz.
	; There are 2M instruction cycles per second.
	; There are 2 instruction cycles per microsecond.
	; CALL and RCALL take 2 instruction cycles.
	; RETURN takes 2 instruction cycles.
	; Therefore, each label must consume exactly 2 instruction cycles fewer than
	; its name implies (the other 2 are consumed by the original CALL/RCALL).

sleep_100ms: ; This label wants 2*100000-2=199998 cycles.
	; 9*20000=180000.
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	rcall sleep_10ms
	; Fallthrough gives another 19998 for 199998.

sleep_10ms: ; This label wants 2*10000-2=19998 cycles.
	; 9*2000=18000.
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	rcall sleep_1ms
	; Fallthrough gives another 1998 for 19998.

sleep_1ms: ; This label wants 2*1000-2=1998 cycles.
	; 9*200=1800.
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	rcall sleep_100us
	; Fallthrough gives another 198 for 1998.

sleep_100us: ; This label wants 2*100-2=198 cycles.
	; 9*20=180.
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	rcall sleep_10us
	; Fallthrough gives another 18 for 198.

sleep_10us: ; This label wants 2*10-2=18 cycles.
	; 4*4=16.
	rcall sleep_2us
	rcall sleep_2us
	rcall sleep_2us
	rcall sleep_2us
	; Fallthrough gives another 2 for 18.

sleep_2us: ; This label wants 2*2-2=2 cycles.
	; 2.
	return

	end
