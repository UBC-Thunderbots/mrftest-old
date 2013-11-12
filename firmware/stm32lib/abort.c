#include <abort.h>
#include <gpio.h>
#include <registers/nvic.h>
#include <registers/otg_fs.h>
#include <sleep.h>

abort_cause_t abort_cause = { .cause = ABORT_CAUSE_UNKNOWN };

static void show_sweeper(void) {
	// Wait a while.
	sleep_ms(3000);

	// Sweep the LEDs three times.
	for (unsigned int i = 0; i < 3; ++i) {
		gpio_set_reset_mask(GPIOB, 1 << 12, 7 << 12);
		sleep_ms(500);
		gpio_set_reset_mask(GPIOB, 1 << 13, 7 << 12);
		sleep_ms(500);
		gpio_set_reset_mask(GPIOB, 1 << 14, 7 << 12);
		sleep_ms(500);
		gpio_set_reset_mask(GPIOB, 1 << 13, 7 << 12);
		sleep_ms(500);
	}

	// Turn the LEDs off and wait another while.
	gpio_set_reset_mask(GPIOB, 0, 7 << 12);
	sleep_ms(1000);
}

static void show_bits(uint8_t bits) {
	gpio_set_reset_mask(GPIOB, (1 << 12) | ((bits & 2) ? (1 << 13) : 0) | ((bits & 1) ? (1 << 14) : 0), 7 << 12);
	sleep_ms(100);
	gpio_reset(GPIOB, 12);
	sleep_ms(900);
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
	OTG_FS_GCCFG.PWRDWN = 0;

	// Turn the three LEDs off.
	gpio_set_reset_mask(GPIOB, 0, 7 << 12);

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
}

