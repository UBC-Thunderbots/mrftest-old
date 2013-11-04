#include <registers/systick.h>
#include <sleep.h>
#include <stdint.h>

void sleep_systick_overflows(unsigned long ticks) {
	{
//		SYST_CVR_t tmp = { 0 };
//		SYST_CVR = tmp;
	}
//	(void) SYST_CSR;
	while (ticks--) {
		while (!SYST_CSR.COUNTFLAG);
	}
}

