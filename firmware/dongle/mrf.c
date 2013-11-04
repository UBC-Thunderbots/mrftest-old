#include "mrf.h"
#include "config.h"
#include <gpio.h>
#include <rcc.h>
#include <registers/spi.h>
#include <sleep.h>

static inline void sleep_50ns(void) {
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
}

void mrf_init(void) {
	// Set bus to reset state
	// PA15 = MRF /CS = 1; deassert chip select
	gpio_set(GPIOA, 15);
	// PB6 = MRF wake = 0; deassert wake.
	// PB7 = MRF /reset = 0; assert reset.
	gpio_set_reset_mask(GPIOB, (1 << 6) | (1 << 7), 0);

	// Reset the module and enable the clock
	rcc_enable(APB2, SPI1);
	rcc_reset(APB2, SPI1);

	if (SPI1.CR1.SPE) {
		// Wait for SPI module to be idle
		while (!SPI1.SR.TXE || SPI1.SR.BSY) {
			if (SPI1.SR.RXNE) {
				(void) SPI1.DR;
			}
		}

		// Disable SPI module
		SPI1.CR1.SPE = 0; // Disable module
	}

	// Configure the SPI module
	{
		SPI_CR2_t tmp = {
			.RXDMAEN = 0, // Receive DMA disabled.
			.TXDMAEN = 0, // Transmit DMA disabled.
			.SSOE = 0, // Do not output hardware slave select; we will use a GPIO for this purpose as we need to toggle it frequently.
			.FRF = 0, // Motorola frame format.
			.ERRIE = 0, // Error interrupt disabled.
			.RXNEIE = 0, // Receive non-empty interrupt disabled.
			.TXEIE = 0, // Transmit empty interrupt disabled.
		};
		SPI1.CR2 = tmp;
	}
	{
		SPI_CR1_t tmp = {
			.CPHA = 0, // Capture on first clock transition, drive new data on second.
			.CPOL = 0, // Clock idles low.
			.MSTR = 1, // Master mode.
			.BR = 2, // Transmission speed is 72 MHz (APB2) ÷ 8 = 9 MHz.
			.SPE = 1, // SPI module is enabled.
			.LSBFIRST = 0, // Most significant bit is sent first.
			.SSI = 1, // Module should assume slave select is high → deasserted → no other master is using the bus.
			.SSM = 1, // Module internal slave select logic is controlled by software (SSI bit).
			.RXONLY = 0, // Transmit and receive.
			.DFF = 0, // Frames are 8 bits wide.
			.CRCNEXT = 0, // Do not transmit a CRC now.
			.CRCEN = 0, // CRC calculation disabled.
			.BIDIMODE = 0, // 2-line bidirectional communication used.
		};
		SPI1.CR1 = tmp;
	}
}

void mrf_release_reset(void) {
	// PB7 = MRF /reset = 1; release reset.
	gpio_set(GPIOB, 7);
}

bool mrf_get_interrupt(void) {
	return gpio_get_input(GPIOC, 12);
}

uint8_t mrf_read_short(mrf_reg_short_t reg) {
	gpio_reset(GPIOA, 15); // PA15 = MRF /CS = 0; assert chip select.
	sleep_50ns();
	SPI1.DR = reg << 1; // Load first byte to send
	while (!SPI1.SR.TXE); // Wait for space in TX buffer (should be almost immediate, 1 byte in shift reg + 1 byte in data reg)
	SPI1.DR = 0; // Load second byte to send
	while (!SPI1.SR.RXNE); // Wait for first inbound byte to be received
	(void) SPI1.DR; // Discard first received byte
	while (!SPI1.SR.RXNE); // Wait for second inbound byte to be received
	uint8_t value = SPI1.DR; // Grab second received byte
	while (SPI1.SR.BSY); // Wait for bus to be fully idle
	sleep_50ns();
	gpio_set(GPIOA, 15); // PA15 = MRF /CS = 1; deassert chip select.
	return value;
}

void mrf_write_short(mrf_reg_short_t reg, uint8_t value) {
	gpio_reset(GPIOA, 15); // PA15 = MRF /CS = 0; assert chip select.
	sleep_50ns();
	SPI1.DR = (reg << 1) | 0x01; // Load first byte to send
	while (!SPI1.SR.TXE); // Wait for space in TX buffer (should be almost immediate, 1 byte in shift reg + 1 byte in data reg)
	SPI1.DR = value; // Load second byte to send
	while (!SPI1.SR.RXNE); // Wait for first inbound byte to be received
	(void) SPI1.DR; // Discard first received byte
	while (!SPI1.SR.RXNE); // Wait for second inbound byte to be received
	(void) SPI1.DR; // Discard second received byte
	while (SPI1.SR.BSY); // Wait for bus to be fully idle
	sleep_50ns();
	gpio_set(GPIOA, 15); // PA15 = MRF /CS = 1; deassert chip select.
}

uint8_t mrf_read_long(mrf_reg_long_t reg) {
	gpio_reset(GPIOA, 15); // PA15 = MRF /CS = 0; assert chip select.
	sleep_50ns();
	SPI1.DR = (reg >> 3) | 0x80; // Load first byte to send (TX count 0→1)
	while (!SPI1.SR.TXE); // Wait for space in TX buffer (TX count 1, RX count 0, should be immediate)
	SPI1.DR = (reg << 5); // Load second byte to send (TX count 1→2)
	while (!SPI1.SR.RXNE); // Wait for first inbound byte to be received (TX count 2→1.1, RX count 0→1)
	(void) SPI1.DR; // Discard first received byte (RX count 1→0)
	while (!SPI1.SR.TXE); // Wait for space in TX buffer (TX count 1.1→1, RX count 0→0.1, should be quite fast)
	SPI1.DR = 0; // Load third byte to send (TX count 1→2)
	while (!SPI1.SR.RXNE); // Wait for second inbound byte to be received (TX count 2→1.1, RX count 0.1→1)
	(void) SPI1.DR; // Discard second received byte (RX count 1→0)
	while (!SPI1.SR.RXNE); // Wait for third inbound byte to be received (TX count 1.1→0, RX count 0→1)
	uint8_t value = SPI1.DR; // Grab third received byte (RX count 1→0)
	while (SPI1.SR.BSY); // Wait for bus to be fully idle
	sleep_50ns();
	gpio_set(GPIOA, 15); // PA15 = MRF /CS = 1; deassert chip select.
	return value;
}

void mrf_write_long(mrf_reg_long_t reg, uint8_t value) {
	gpio_reset(GPIOA, 15); // PA15 = MRF /CS = 0; assert chip select.
	sleep_50ns();
	SPI1.DR = (reg >> 3) | 0x80; // Load first byte to send (TX count 0→1)
	while (!SPI1.SR.TXE); // Wait for space in TX buffer (TX count 1, RX count 0, should be immediate)
	SPI1.DR = (reg << 5) | 0x10; // Load second byte to send (TX count 1→2)
	while (!SPI1.SR.RXNE); // Wait for first inbound byte to be received (TX count 2→1.1, RX count 0→1)
	(void) SPI1.DR; // Discard first received byte (RX count 1→0)
	while (!SPI1.SR.TXE); // Wait for space in TX buffer (TX count 1.1→1, RX count 0→0.1, should be quite fast)
	SPI1.DR = value; // Load third byte to send (TX count 1→2)
	while (!SPI1.SR.RXNE); // Wait for second inbound byte to be received (TX count 2→1.1, RX count 0.1→1)
	(void) SPI1.DR; // Discard second received byte (RX count 1→0)
	while (!SPI1.SR.RXNE); // Wait for third inbound byte to be received (TX count 1.1→0, RX count 0→1)
	(void) SPI1.DR; // Discard third received byte (RX count 1→0)
	while (SPI1.SR.BSY); // Wait for bus to be fully idle
	sleep_50ns();
	gpio_set(GPIOA, 15); // PA15 = MRF /CS = 1; deassert chip select.
}

void mrf_common_init(void) {
	mrf_write_short(MRF_REG_SHORT_SOFTRST, 0x07);
	mrf_write_short(MRF_REG_SHORT_PACON2, 0x98);
	mrf_write_short(MRF_REG_SHORT_TXPEND, 0x7C);
	mrf_write_short(MRF_REG_SHORT_TXTIME, 0x38);
	mrf_write_short(MRF_REG_SHORT_TXSTBL, 0x95);
	mrf_write_long(MRF_REG_LONG_RFCON0, 0x03);
	mrf_write_long(MRF_REG_LONG_RFCON1, 0x02);
	mrf_write_long(MRF_REG_LONG_RFCON2, 0x80);
	mrf_write_long(MRF_REG_LONG_RFCON6, 0x90);
	mrf_write_long(MRF_REG_LONG_RFCON7, 0x80);
	mrf_write_long(MRF_REG_LONG_RFCON8, 0x10);
	mrf_write_long(MRF_REG_LONG_SLPCON0, 0x03);
	mrf_write_long(MRF_REG_LONG_SLPCON1, 0x21);
	mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x61);
	mrf_write_short(MRF_REG_SHORT_BBREG2, 0xB8);
	mrf_write_short(MRF_REG_SHORT_CCAEDTH, 0xDD);
	mrf_write_short(MRF_REG_SHORT_BBREG6, 0x40);
	mrf_write_long(MRF_REG_LONG_RFCON0, ((config.channel - 0x0B) << 4) | 0x03);
	mrf_write_long(MRF_REG_LONG_RFCON3, 0x28);
	mrf_write_short(MRF_REG_SHORT_RFCTL, 0x04);
	mrf_write_short(MRF_REG_SHORT_RFCTL, 0x00);
	sleep_us(200);
	if (config.symbol_rate) {
		mrf_write_short(MRF_REG_SHORT_BBREG0, 0x01);
		mrf_write_short(MRF_REG_SHORT_BBREG3, 0x34);
		mrf_write_short(MRF_REG_SHORT_BBREG4, 0x5C);
		mrf_write_short(MRF_REG_SHORT_SOFTRST, 0x02);
	}
	mrf_write_short(MRF_REG_SHORT_PANIDL, config.pan_id);
	mrf_write_short(MRF_REG_SHORT_PANIDH, config.pan_id >> 8);
	mrf_write_short(MRF_REG_SHORT_EADR0, config.mac_address);
	mrf_write_short(MRF_REG_SHORT_EADR1, config.mac_address >> 8);
	mrf_write_short(MRF_REG_SHORT_EADR2, config.mac_address >> 16);
	mrf_write_short(MRF_REG_SHORT_EADR3, config.mac_address >> 24);
	mrf_write_short(MRF_REG_SHORT_EADR4, config.mac_address >> 32);
	mrf_write_short(MRF_REG_SHORT_EADR5, config.mac_address >> 40);
	mrf_write_short(MRF_REG_SHORT_EADR6, config.mac_address >> 48);
	mrf_write_short(MRF_REG_SHORT_EADR7, config.mac_address >> 56);
	mrf_analogue_off();
	mrf_write_short(MRF_REG_SHORT_TRISGPIO, 0x3F);
}

void mrf_analogue_off(void) {
	mrf_write_short(MRF_REG_SHORT_GPIO, 0x00);
	mrf_write_long(MRF_REG_LONG_TESTMODE, 0x08);
}

void mrf_analogue_rx(void) {
	mrf_write_short(MRF_REG_SHORT_GPIO, 0x04);
	mrf_write_long(MRF_REG_LONG_TESTMODE, 0x08);
}

void mrf_analogue_txrx(void) {
	mrf_write_short(MRF_REG_SHORT_GPIO, 0x08);
	mrf_write_long(MRF_REG_LONG_TESTMODE, 0x0F);
}

