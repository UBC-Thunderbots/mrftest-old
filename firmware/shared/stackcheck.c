#include "pins.h"
#include <pic18fregs.h>

void stackcheck_failed(void) __naked {
	LAT_LED1 = 1;
	LAT_LED2 = 1;
	LAT_LED3 = 1;
#ifdef LAT_LED4
	LAT_LED4 = 1;
#endif
	INTCONbits.GIEH = 0;
	for (;;) {
		Sleep();
	}
}

