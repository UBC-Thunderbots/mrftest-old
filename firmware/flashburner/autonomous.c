#include "autonomous.h"
#include "spi.h"
#include <assert.h>
#include <deferred.h>
#include <rcc.h>
#include <registers.h>
#include <sleep.h>
#include <unused.h>

#define PAGE_BYTES 256
#define NUM_PAGES (2 * 1024 * 1024 / PAGE_BYTES)

static bool running = false, stopping = false;
static bool boot_after;
static deferred_fn_t deferred = DEFERRED_FN_INIT;
static unsigned int next_page;

static void stop(bool successful) {
	// On failure, light up the error LED.
	if (!successful) {
		GPIOB_BSRR = GPIO_BS(14);
	}

	// Deassert chip select.
	spi_external_ops.deassert_cs();

	// Tristate MOSI, MISO, clock, and chip select.
	GPIOA_MODER &= ~MODER_MSK(15);
	GPIOB_MODER &= ~MODER_MSK(5) & ~MODER_MSK(4) & ~MODER_MSK(3) & ~MODER_MSK(2);

	if (successful && boot_after) {
		// Boot the FPGA by tristating PROGRAM_B.
		GPIOA_MODER &= ~MODER_MSK(14);
	} else {
		// Power down the board by grounding power control.
		GPIOD_BSRR = GPIO_BR(2);
	}

	// Account.
	stopping = true;

	// Start the timer.
	TIM5_CNT = 50000; // Set count.
	TIM5_CR1 |= CEN; // Enable timer.
}

static uint8_t read_status_register(void) {
	spi_external_ops.assert_cs();
	spi_external_ops.transceive_byte(0x05);
	uint8_t status = spi_external_ops.transceive_byte(0);
	spi_external_ops.deassert_cs();
	return status;
}

static void write_next_page(void);
static void verify_next_page(void *cookie);

static void erase_poll_status(void *UNUSED(cookie)) {
	if (read_status_register() & 1) {
		// Still busy.
		deferred_fn_register(&deferred, &erase_poll_status, 0);
		return;
	}

	// Start a write.
	next_page = 0;
	write_next_page();
}

static void write_poll_status(void *UNUSED(cookie)) {
	if (read_status_register() & 1) {
		// Still busy.
		deferred_fn_register(&deferred, &write_poll_status, 0);
		return;
	}

	if (next_page < NUM_PAGES) {
		write_next_page();
	} else {
		next_page = 0;
		verify_next_page(0);
	}
}

static void write_next_page(void) {
	// Send write enable.
	spi_external_ops.assert_cs();
	spi_external_ops.transceive_byte(0x06);
	spi_external_ops.deassert_cs();

	// Interleave a read and a write from the onboard Flash to the target.
	spi_internal_ops.assert_cs();
	spi_internal_ops.transceive_byte(0x0B);
	spi_internal_ops.transceive_byte(next_page >> 8);
	spi_internal_ops.transceive_byte(next_page);
	spi_internal_ops.transceive_byte(0);
	spi_internal_ops.transceive_byte(0);
	spi_external_ops.assert_cs();
	spi_external_ops.transceive_byte(0x02);
	spi_external_ops.transceive_byte(next_page >> 8);
	spi_external_ops.transceive_byte(next_page);
	spi_external_ops.transceive_byte(0);
	for (unsigned int i = 0; i < 256; ++i) {
		spi_external_ops.transceive_byte(spi_internal_ops.transceive_byte(0));
	}
	spi_external_ops.deassert_cs();
	spi_internal_ops.deassert_cs();

	// Increment next page number.
	++next_page;

	// Start polling for write complete.
	deferred_fn_register(&deferred, &write_poll_status, 0);
}

static void verify_next_page(void *UNUSED(cookie)) {
	// Interleave a read and a write from the two Flash chips.
	bool ok = true;
	spi_internal_ops.assert_cs();
	spi_internal_ops.transceive_byte(0x0B);
	spi_internal_ops.transceive_byte(next_page >> 8);
	spi_internal_ops.transceive_byte(next_page);
	spi_internal_ops.transceive_byte(0);
	spi_internal_ops.transceive_byte(0);
	spi_external_ops.assert_cs();
	spi_external_ops.transceive_byte(0x0B);
	spi_external_ops.transceive_byte(next_page >> 8);
	spi_external_ops.transceive_byte(next_page);
	spi_external_ops.transceive_byte(0);
	spi_external_ops.transceive_byte(0);
	for (unsigned int i = 0; i < 256; ++i) {
		if (spi_internal_ops.transceive_byte(0) != spi_external_ops.transceive_byte(0)) {
			ok = false;
		}
	}
	spi_internal_ops.deassert_cs();
	spi_external_ops.deassert_cs();

	// Increment page number.
	++next_page;

	if (ok) {
		if (next_page < NUM_PAGES) {
			// There are more pages to verify.
			deferred_fn_register(&deferred, &verify_next_page, 0);
		} else {
			// We have reached the end of the Flash.
			stop(true);
		}
	} else {
		// Verification failed.
		stop(false);
	}
}

bool autonomous_is_running(void) {
	return running;
}

void timer5_interrupt_vector(void) {
	// Clear interrupt.
	TIM5_SR = 0;

	if (stopping) {
		// Power down timer 5.
		rcc_disable(APB1, 3);

		// Tristate PROGRAM_B and power control.
		GPIOA_MODER &= ~MODER_MSK(14);
		GPIOD_MODER &= ~MODER_MSK(2);

		// Turn off the activity LED, but not the error LED if it is on.
		GPIOB_BSRR = GPIO_BR(13);

		// Account.
		running = false;
		stopping = false;
	} else {
		// We have just finished waiting for the Flash to be ready to use.
		// Kick off the autonomous operation.
		// Drive the SPI lines.
		spi_external_ops.drive_bus();

		// Wait a moment for them to stabilize.
		sleep_us(20);

		// Read and check the JEDEC ID.
		spi_external_ops.assert_cs();
		spi_external_ops.transceive_byte(0x9F);
		uint8_t mfgr = spi_external_ops.transceive_byte(0);
		uint8_t type = spi_external_ops.transceive_byte(0);
		uint8_t capacity = spi_external_ops.transceive_byte(0);
		spi_external_ops.deassert_cs();
		if (mfgr != 0xEF || type != 0x40 || capacity != 0x15) {
			stop(false);
			return;
		}

		// Read and check the status register.
		uint8_t status = read_status_register();
		if (status & 1) {
			stop(false);
			return;
		}

		// Start an erase.
		spi_external_ops.assert_cs();
		spi_external_ops.transceive_byte(0x06);
		spi_external_ops.deassert_cs();
		spi_external_ops.assert_cs();
		spi_external_ops.transceive_byte(0xC7);
		spi_external_ops.deassert_cs();

		// Poll the status register until the erase finishes.
		deferred_fn_register(&deferred, &erase_poll_status, 0);
	}
}

void autonomous_start(bool boot) {
	// Sanity check.
	assert(!running);

	// Record variables.
	running = true;
	boot_after = boot;

	// Enable activity LED and disable error LED.
	GPIOB_BSRR = GPIO_BS(13) | GPIO_BR(14);

	// Assert PROGRAM_B and power control.
	GPIOA_BSRR = GPIO_BR(14);
	GPIOA_MODER = (GPIOA_MODER & ~MODER_MSK(14)) | MODER(14, 1);
	GPIOD_BSRR = GPIO_BS(2);
	GPIOD_MODER = (GPIOD_MODER & ~MODER_MSK(2)) | MODER(2, 1);

	// Configure timer 5 to count down 100 milliseconds, until the power is on.
	// Timer 5 is on APB1, which runs at 36 MHz.
	// APB1 is divided, so timers run at 2× speed, or 72 MHz.
	// Divide by 720 to get tens of microseconds, and count down from 10,000.
	rcc_enable(APB1, 3);
	TIM5_CR1 = 0 // Auto reload not buffered, in one direction, updates not disabled, counter disabled for now
		| DIR // Counter counts down
		| OPM // Counter counts once, not continuously
		| URS; // Only underflow generates an interrupt
	TIM5_SMCR = 0; // Slave mode disabled
	TIM5_DIER = UIE; // Enable interrupt on timer update
	TIM5_CNT = 0; // Clear timer
	TIM5_PSC = 719; // Set prescale 1:720
	TIM5_ARR = 1; // An auto-reload value of zero does not work; a value of 1, however, makes sure TIM5_CNT is very small when the counter stops, so it is less than future requests
	TIM5_CR1 |= CEN; // Enable counter for one tick after which an interrupt will be delivered (not doing this appears to break the first enablement of the timer by instantly delivering an interrupt for no apparent reason)
	while (!(TIM5_SR & UIF)); // Wait for interrupt
	TIM5_SR = 0; // Clear interrupt
	TIM5_CNT = 9999; // Set count.
	TIM5_CR1 |= CEN; // Enable timer.
	NVIC_ICPR[50 / 32] = 1 << (50 % 32); // Clear pending interrupt in NVIC
	NVIC_ISER[50 / 32] = 1 << (50 % 32); // SETENA50 = 1; enable timer 5 interrupt
}

