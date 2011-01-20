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
		; 0 = ch
		; 1 = crc[high]
		; PRODL = crc[low]
		movff 0, _POSTDEC1
		movff 1, _POSTDEC1

		; Load regs, leaving crc[low] in W.
		movlw 3
		movff _PLUSW1, 0
		movlw 4
		movff _PLUSW1, 1
		movlw 5
		movf _PLUSW1, W
		movwf _PRODL

		; ch ^= crc
		xorwf 0, F

		; ch ^= ch << 4
		swapf 0, W
		andlw 0xF0
		xorwf 0, F

		; crc = crc >> 8
		; rename registers:
		; 0 = ch
		; 1 = crc[low]
		; PRODL = crc[high]

		; crc |= ch << 8 (note crc[high] = 0 after crc >>= 8)
		movff 0, _PRODL

		; crc ^= ch >> 4
		swapf 0, W
		andlw 0x0F
		xorwf 1, F

		; rename registers:
		; 0 = ch ROL 3
		; 1 = crc[low]
		; PRODL = crc[high]
		swapf 0, F
		rrncf 0, F

		; crc ^= ch << 3
		; high part
		movf 0, W
		andlw 0x07
		xorwf _PRODL, F
		; low part - leave it in W
		movf 0, W
		andlw 0xF8
		xorwf 1, W

		; return crc
		movff _PREINC1, 1
		movff _PREINC1, 0
		return
	__endasm;
}
#endif

uint16_t crc_update_block(uint16_t crc, __data const void *pch, uint8_t len) {
	__data const uint8_t *pb = pch;

	while (len--) {
		crc = crc_update(crc, *pb++);
	}

	return crc;
}

