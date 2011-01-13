#include "crc.h"
#include <pic18fregs.h>

/* These are equivalent. The assembly is over twice as fast. */
#if 0
uint16_t crc_update(uint16_t crc, uint8_t ch) {
	ch ^= crc;
	ch ^= ch << 4;
	crc = crc >> 8;
	crc |= ch << 8;
	crc ^= ch << 3;
	crc ^= ch >> 4;
	return crc;
}
#else
uint16_t crc_update(uint16_t crc, uint8_t ch) __naked {
	__asm
		; r0x00 = ch
		; r0x01 = crc[high]
		; PRODL = crc[low]
		movff r0x00, POSTDEC1
		movff r0x01, POSTDEC1

		; Load regs, leaving crc[low] in W.
		movlw 3
		movff _PLUSW1, r0x00
		movlw 4
		movff _PLUSW1, r0x01
		movlw 5
		movf _PLUSW1, W
		movwf PRODL

		; ch ^= crc
		xorwf r0x00, F

		; ch ^= ch << 4
		swapf r0x00, W
		andlw 0xF0
		xorwf r0x00, F

		; crc = crc >> 8
		; rename registers:
		; r0x00 = ch
		; r0x01 = crc[low]
		; PRODL = crc[high]

		; crc |= ch << 8 (note crc[high] = 0 after crc >>= 8)
		movff r0x00, PRODL

		; crc ^= ch >> 4
		swapf r0x00, W
		andlw 0x0F
		xorwf r0x01, F

		; rename registers:
		; r0x00 = ch ROL 3
		; r0x01 = crc[low]
		; PRODL = crc[high]
		swapf r0x00, F
		rrncf r0x00, F

		; crc ^= ch << 3
		; high part
		movf r0x00, W
		andlw 0x07
		xorwf PRODL, F
		; low part - leave it in W
		movf r0x00, W
		andlw 0xF8
		xorwf r0x01, W

		; return crc
		movff PREINC1, r0x01
		movff PREINC1, r0x00
		return
	__endasm;
}
#endif

