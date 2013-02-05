#include "mrf.h"
#include "config.h"
#include "rcc.h"
#include "registers.h"
#include "sleep.h"

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
	GPIOA_BSRR = GPIO_BS(15); // PA15 = MRF /CS = 1; deassert chip select
	GPIOB_BSRR = GPIO_BR(6) // PB6 = MRF wake = 0; deassert wake
		| GPIO_BR(7); // PB7 = MRF /reset = 0; assert reset

	// Reset the module and enable the clock
	rcc_enable(APB2, 12);

	if (SPI1_CR1 & SPE) {
		// Wait for SPI module to be idle
		while (!(SPI1_SR & TXE) || (SPI1_SR & SPI_BSY)) {
			if (SPI1_SR & RXNE) {
				(void) SPI1_DR;
			}
		}

		// Disable SPI module
		SPI1_CR1 &= ~SPE; // Disable module
	}

	// Configure the SPI module
	SPI1_CR2 = 0; // Motorola frame format, hardware chip select output disabled, DMA disabled
	SPI1_CR1 = 0 // Bidirectional mode disabled, CRC calculation disabled, 8 bit frames, receive-only mode disabled, MSb sent first, clock idles low, capture is on rising edge
		| SSM // Software slave select management selected
		| SSI // Assume slave select is deasserted (no other master is transmitting)
		| SPE // Module is enabled
		| BR(2) // Transmission speed is 72 MHz (APB2) ÷ 8 = 9 MHz
		| MSTR; // Master mode
}

void mrf_release_reset(void) {
	GPIOB_BSRR = GPIO_BS(7); // PB7 = MRF /reset = 1; release reset
}

bool mrf_get_interrupt(void) {
	return !!(GPIOC_IDR & (1 << 12));
}

uint8_t mrf_read_short(mrf_reg_short_t reg) {
	GPIOA_BSRR = GPIO_BR(15); // PA15 = MRF /CS = 0; assert chip select
	sleep_50ns();
	SPI1_DR = reg << 1; // Load first byte to send
	while (!(SPI1_SR & TXE)); // Wait for space in TX buffer (should be almost immediate, 1 byte in shift reg + 1 byte in data reg)
	SPI1_DR = 0; // Load second byte to send
	while (!(SPI1_SR & RXNE)); // Wait for first inbound byte to be received
	(void) SPI1_DR; // Discard first received byte
	while (!(SPI1_SR & RXNE)); // Wait for second inbound byte to be received
	uint8_t value = SPI1_DR; // Grab second received byte
	while (SPI1_SR & SPI_BSY); // Wait for bus to be fully idle
	sleep_50ns();
	GPIOA_BSRR = GPIO_BS(15); // PA15 = MRF /CS = 1; deassert chip select
	return value;
}

void mrf_write_short(mrf_reg_short_t reg, uint8_t value) {
	GPIOA_BSRR = GPIO_BR(15); // PA15 = MRF /CS = 0; assert chip select
	sleep_50ns();
	SPI1_DR = (reg << 1) | 0x01; // Load first byte to send
	while (!(SPI1_SR & TXE)); // Wait for space in TX buffer (should be almost immediate, 1 byte in shift reg + 1 byte in data reg)
	SPI1_DR = value; // Load second byte to send
	while (!(SPI1_SR & RXNE)); // Wait for first inbound byte to be received
	(void) SPI1_DR; // Discard first received byte
	while (!(SPI1_SR & RXNE)); // Wait for second inbound byte to be received
	(void) SPI1_DR; // Discard second received byte
	while (SPI1_SR & SPI_BSY); // Wait for bus to be fully idle
	sleep_50ns();
	GPIOA_BSRR = GPIO_BS(15); // PA15 = MRF /CS = 1; deassert chip select
}

uint8_t mrf_read_long(mrf_reg_long_t reg) {
	GPIOA_BSRR = GPIO_BR(15); // PA15 = MRF /CS = 0; assert chip select
	sleep_50ns();
	SPI1_DR = (reg >> 3) | 0x80; // Load first byte to send (TX count 0→1)
	while (!(SPI1_SR & TXE)); // Wait for space in TX buffer (TX count 1, RX count 0, should be immediate)
	SPI1_DR = (reg << 5); // Load second byte to send (TX count 1→2)
	while (!(SPI1_SR & RXNE)); // Wait for first inbound byte to be received (TX count 2→1.1, RX count 0→1)
	(void) SPI1_DR; // Discard first received byte (RX count 1→0)
	while (!(SPI1_SR & TXE)); // Wait for space in TX buffer (TX count 1.1→1, RX count 0→0.1, should be quite fast)
	SPI1_DR = 0; // Load third byte to send (TX count 1→2)
	while (!(SPI1_SR & RXNE)); // Wait for second inbound byte to be received (TX count 2→1.1, RX count 0.1→1)
	(void) SPI1_DR; // Discard second received byte (RX count 1→0)
	while (!(SPI1_SR & RXNE)); // Wait for third inbound byte to be received (TX count 1.1→0, RX count 0→1)
	uint8_t value = SPI1_DR; // Grab third received byte (RX count 1→0)
	while (SPI1_SR & SPI_BSY); // Wait for bus to be fully idle
	sleep_50ns();
	GPIOA_BSRR = GPIO_BS(15); // PA15 = MRF /CS = 1; deassert chip select
	return value;
}

void mrf_write_long(mrf_reg_long_t reg, uint8_t value) {
	GPIOA_BSRR = GPIO_BR(15); // PA15 = MRF /CS = 0; assert chip select
	sleep_50ns();
	SPI1_DR = (reg >> 3) | 0x80; // Load first byte to send (TX count 0→1)
	while (!(SPI1_SR & TXE)); // Wait for space in TX buffer (TX count 1, RX count 0, should be immediate)
	SPI1_DR = (reg << 5) | 0x10; // Load second byte to send (TX count 1→2)
	while (!(SPI1_SR & RXNE)); // Wait for first inbound byte to be received (TX count 2→1.1, RX count 0→1)
	(void) SPI1_DR; // Discard first received byte (RX count 1→0)
	while (!(SPI1_SR & TXE)); // Wait for space in TX buffer (TX count 1.1→1, RX count 0→0.1, should be quite fast)
	SPI1_DR = value; // Load third byte to send (TX count 1→2)
	while (!(SPI1_SR & RXNE)); // Wait for second inbound byte to be received (TX count 2→1.1, RX count 0.1→1)
	(void) SPI1_DR; // Discard second received byte (RX count 1→0)
	while (!(SPI1_SR & RXNE)); // Wait for third inbound byte to be received (TX count 1.1→0, RX count 0→1)
	(void) SPI1_DR; // Discard third received byte (RX count 1→0)
	while (SPI1_SR & SPI_BSY); // Wait for bus to be fully idle
	sleep_50ns();
	GPIOA_BSRR = GPIO_BS(15); // PA15 = MRF /CS = 1; deassert chip select
}

void mrf_common_init(void) {
	mrf_write_short(MRF_REG_SHORT_SOFTRST, 0x07);
	mrf_write_short(MRF_REG_SHORT_PACON2, 0x98);
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
	mrf_write_short(MRF_REG_SHORT_BBREG2, 0x80);
	mrf_write_short(MRF_REG_SHORT_CCAEDTH, 0x60);
	mrf_write_short(MRF_REG_SHORT_BBREG6, 0x40);
	mrf_write_long(MRF_REG_LONG_RFCON0, ((config.channel - 0x0B) << 4) | 0x03);
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

