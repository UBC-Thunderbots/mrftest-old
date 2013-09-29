#include <abort.h>
#include <registers.h>
#include <sleep.h>

abort_cause_t abort_cause = { .cause = ABORT_CAUSE_UNKNOWN };

static void show_sweeper(void) {
	// Wait a while.
	sleep_ms(3000);

	// Sweep the LEDs three times.
	for (unsigned int i = 0; i < 3; ++i) {
		GPIOB_BSRR = GPIO_BS(12) | GPIO_BR(13) | GPIO_BR(14);
		sleep_ms(500);
		GPIOB_BSRR = GPIO_BR(12) | GPIO_BS(13) | GPIO_BR(14);
		sleep_ms(500);
		GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BS(14);
		sleep_ms(500);
		GPIOB_BSRR = GPIO_BR(12) | GPIO_BS(13) | GPIO_BR(14);
		sleep_ms(500);
	}

	// Turn the LEDs off and wait another while.
	GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BR(14);
	sleep_ms(1000);
}

static void show_bits(uint8_t bits) {
#if 0
	// Show the bottom two bits on LEDs 2 and 3, and raise LED 1 (“clock”).
	GPIOB_BSRR = GPIO_BS(12) | ((bits & 2) ? GPIO_BS(13) : GPIO_BR(13)) | ((bits & 1) ? GPIO_BS(14) : GPIO_BR(14));
	sleep_ms(500);
	// Turn off LED 1.
	GPIOB_BSRR = GPIO_BR(12);
	sleep_ms(500);
#elif 0
	GPIOB_BSRR = GPIO_BR(12) | ((bits & 2) ? GPIO_BS(13) : GPIO_BR(13)) | ((bits & 1) ? GPIO_BS(14) : GPIO_BR(14));
	sleep_ms(333);
	GPIOB_BSRR = GPIO_BS(12);
	sleep_ms(333);
	GPIOB_BSRR = GPIO_BR(12);
	sleep_ms(333);
#else
	GPIOB_BSRR = GPIO_BS(12) | ((bits & 2) ? GPIO_BS(13) : GPIO_BR(13)) | ((bits & 1) ? GPIO_BS(14) : GPIO_BR(14));
	sleep_ms(100);
	GPIOB_BSRR = GPIO_BR(12);
	sleep_ms(900);
#endif
}

static void show_byte(uint8_t byte) {
	show_bits(byte >> 6);
	show_bits(byte >> 4);
	show_bits(byte >> 2);
	show_bits(byte);
}

// This overrides the standard library’s abort function.
void abort(void) {
	// Scramble and disable every interrupt.
	for (unsigned int i = 0; i < 16; ++i) {
		NVIC_ICER[i] = 0xFFFFFFFF;
	}

	// Power down the USB engine to disconnect from the host.
	OTG_FS_GCCFG &= ~PWRDWN;

	// Turn the three LEDs off.
	GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BR(14);

	// Go through the loop rendering the abort cause on the LEDs.
	for (;;) {
		show_sweeper();
		show_byte(abort_cause.cause);
		for (unsigned int i = 0; i < 3; ++i) {
			for (unsigned int j = 3; j < 4; --j) {
				show_byte(abort_cause.detail[i] >> (8 * j));
			}
		}
	}

	// Turn the three LEDs on.
	GPIOB_BSRR = GPIO_BS(14) | GPIO_BS(13) | GPIO_BS(12);

	// Flash the LEDs forever.
	for (;;) {
		sleep_ms(1000);
		GPIOB_BSRR = GPIO_BR(14) | GPIO_BR(13) | GPIO_BR(12);
		sleep_ms(1000);
		GPIOB_BSRR = GPIO_BS(14) | GPIO_BS(13) | GPIO_BS(12);
	}
}

