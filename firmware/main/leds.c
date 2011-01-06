#include "leds.h"
#include "pins.h"
#include <pic18fregs.h>

void leds_show_number(unsigned char n) {
	if (n & 0x01) {
		LAT_LED1 = 1;
	} else {
		LAT_LED1 = 0;
	}
	if (n & 0x02) {
		LAT_LED2 = 1;
	} else {
		LAT_LED2 = 0;
	}
	if (n & 0x04) {
		LAT_LED3 = 1;
	} else {
		LAT_LED3 = 0;
	}
	if (n & 0x08) {
		LAT_LED4 = 1;
	} else {
		LAT_LED4 = 0;
	}
}

