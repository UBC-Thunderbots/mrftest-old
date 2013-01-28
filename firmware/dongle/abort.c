#include "registers.h"
#include "sleep.h"

void abort(void) {
	// Scramble and disable every interrupt.
	for (unsigned int i = 0; i < 16; ++i) {
		NVIC_ICER[i] = 0xFFFFFFFF;
	}

	// Power down the USB engine to disconnect from the host.
	OTG_FS_GCCFG &= ~PWRDWN;

	// Turn the three LEDs on.
	GPIOB_BSRR = GPIO_BS(14) | GPIO_BS(13) | GPIO_BS(12);

	// Flash the LEDs forever.
	for (;;) {
		sleep_1ms(1000);
		GPIOB_BSRR = GPIO_BR(14) | GPIO_BR(13) | GPIO_BR(12);
		sleep_1ms(1000);
		GPIOB_BSRR = GPIO_BS(14) | GPIO_BS(13) | GPIO_BS(12);
	}
}

