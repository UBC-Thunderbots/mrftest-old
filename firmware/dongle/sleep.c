#include "registers.h"
#include "sleep.h"
#include "stdint.h"

void sleep_systick_overflows(unsigned long ticks) {
	SCS_STCVR = 0;
	(void) SCS_STCSR;
	while (ticks--) {
		while (!(SCS_STCSR & 0x00010000));
	}
}

