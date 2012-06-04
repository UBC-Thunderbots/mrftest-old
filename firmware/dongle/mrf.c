#include "mrf.h"
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
	GPIOA_ODR |= 1 << 15; // PA15 = MRF /CS = 1; deassert chip select
	GPIOB_ODR &= ~(1 << 6); // PB6 = MRF wake = 0; deassert wake
	GPIOB_ODR &= ~(1 << 7); // PB7 = MRF /reset = 0; assert reset

	// Reset the module and enable the clock
	RCC_APB2RSTR |= 1 << 12; // SPI1RST = 1; reset SPI 1
	asm volatile("dsb");
	asm volatile("nop");
	RCC_APB2ENR |= 1 << 12; // SPI1EN = 1; enable clock to SPI 1
	asm volatile("dsb");
	asm volatile("nop");
	RCC_APB2RSTR &= ~(1 << 12); // SPI1RST = 0; release SPI 1 from reset
	asm volatile("dsb");
	asm volatile("nop");

	if (SPI1_CR1 & (1 << 6) /* SPE */) {
		// Wait for SPI module to be idle
		while (!(SPI1_SR & (1 << 1) /* TXE */) || (SPI1_SR & (1 << 7) /* BSY */)) {
			if (SPI1_SR & (1 << 0)) {
				(void) SPI1_DR;
			}
		}

		// Disable SPI module
		SPI1_CR1 &= ~(1 << 6); // SPE = 0; disable module
	}

	// Configure the SPI module
	SPI1_CR2 = (SPI1_CR2 & 0b1111111100001000) // Reserved
		| (0 << 4) // Motorola frame format
		| (0 << 2) // SPOE = 0; hardware chip select output is disabled
		| (0 << 1) // TXDMAEN = 0; no transmit DMA
		| (0 << 0); // RXDMAEN = 0; no receive DMA
	SPI1_CR1 = (0 << 15) // BIDIMODE = 0; 2-line unidirectional mode
		| (0 << 14) // BIDIOE = 0; ignored in unidirectional mode
		| (0 << 13) // CRCEN = 0; hardware CRC calculation disabled
		| (0 << 12) // CRCNEXT = 0; next transfer is not a CRC
		| (0 << 11) // DFF = 0; data frames are 8 bits wide
		| (0 << 10) // RXONLY = 0; module both transmits and receives
		| (1 << 9) // SSM = 1; software slave select management selected
		| (1 << 8) // SSI = 1; assume slave select is deasserted (no other master is transmitting)
		| (0 << 7) // LSBFIRST = 0; data is transferred MSb first
		| (1 << 6) // SPE = 1; module is enabled
		| (2 << 3) // BR = 2; transmission speed is 72 MHz (APB2) ÷ 8 = 9 MHz
		| (1 << 2) // MSTR = 1; master mode
		| (0 << 1) // CPOL = 0; clock idles low
		| (0 << 0); // CPHA = 0; capture is on rising (first) edge while advance is on falling (second) edge
}

void mrf_release_reset(void) {
	GPIOB_ODR |= 1 << 7; // PB7 = MRF /reset = 1; release reset
}

bool mrf_get_interrupt(void) {
	return !!(GPIOC_IDR & (1 << 12));
}

uint8_t mrf_read_short(mrf_reg_short_t reg) {
	GPIOA_ODR &= ~(1 << 15); // PA15 = MRF /CS = 0; assert chip select
	sleep_50ns();
	SPI1_DR = reg << 1; // Load first byte to send
	while (!(SPI1_SR & (1 << 1) /* TXE */)); // Wait for space in TX buffer (should be almost immediate, 1 byte in shift reg + 1 byte in data reg)
	SPI1_DR = 0; // Load second byte to send
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for first inbound byte to be received
	(void) SPI1_DR; // Discard first received byte
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for second inbound byte to be received
	uint8_t value = SPI1_DR; // Grab second received byte
	while (SPI1_SR & (1 << 7) /* BSY */); // Wait for bus to be fully idle
	sleep_50ns();
	GPIOA_ODR |= 1 << 15; // PA15 = MRF /CS = 1; deassert chip select
	return value;
}

void mrf_write_short(mrf_reg_short_t reg, uint8_t value) {
	GPIOA_ODR &= ~(1 << 15); // PA15 = MRF /CS = 0; assert chip select
	sleep_50ns();
	SPI1_DR = (reg << 1) | 0x01; // Load first byte to send
	while (!(SPI1_SR & (1 << 1) /* TXE */)); // Wait for space in TX buffer (should be almost immediate, 1 byte in shift reg + 1 byte in data reg)
	SPI1_DR = value; // Load second byte to send
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for first inbound byte to be received
	(void) SPI1_DR; // Discard first received byte
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for second inbound byte to be received
	(void) SPI1_DR; // Discard second received byte
	while (SPI1_SR & (1 << 7) /* BSY */); // Wait for bus to be fully idle
	sleep_50ns();
	GPIOA_ODR |= 1 << 15; // PA15 = MRF /CS = 1; deassert chip select
}

uint8_t mrf_read_long(mrf_reg_long_t reg) {
	GPIOA_ODR &= ~(1 << 15); // PA15 = MRF /CS = 0; assert chip select
	sleep_50ns();
	SPI1_DR = (reg >> 3) | 0x80; // Load first byte to send (TX count 0→1)
	while (!(SPI1_SR & (1 << 1) /* TXE */)); // Wait for space in TX buffer (TX count 1, RX count 0, should be immediate)
	SPI1_DR = (reg << 5); // Load second byte to send (TX count 1→2)
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for first inbound byte to be received (TX count 2→1.1, RX count 0→1)
	(void) SPI1_DR; // Discard first received byte (RX count 1→0)
	while (!(SPI1_SR & (1 << 1) /* TXE */)); // Wait for space in TX buffer (TX count 1.1→1, RX count 0→0.1, should be quite fast)
	SPI1_DR = 0; // Load third byte to send (TX count 1→2)
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for second inbound byte to be received (TX count 2→1.1, RX count 0.1→1)
	(void) SPI1_DR; // Discard second received byte (RX count 1→0)
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for third inbound byte to be received (TX count 1.1→0, RX count 0→1)
	uint8_t value = SPI1_DR; // Grab third received byte (RX count 1→0)
	while (SPI1_SR & (1 << 7) /* BSY */); // Wait for bus to be fully idle
	sleep_50ns();
	GPIOA_ODR |= 1 << 15; // PA15 = MRF /CS = 1; deassert chip select
	return value;
}

void mrf_write_long(mrf_reg_long_t reg, uint8_t value) {
	GPIOA_ODR &= ~(1 << 15); // PA15 = MRF /CS = 0; assert chip select
	sleep_50ns();
	SPI1_DR = (reg >> 3) | 0x80; // Load first byte to send (TX count 0→1)
	while (!(SPI1_SR & (1 << 1) /* TXE */)); // Wait for space in TX buffer (TX count 1, RX count 0, should be immediate)
	SPI1_DR = (reg << 5) | 0x10; // Load second byte to send (TX count 1→2)
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for first inbound byte to be received (TX count 2→1.1, RX count 0→1)
	(void) SPI1_DR; // Discard first received byte (RX count 1→0)
	while (!(SPI1_SR & (1 << 1) /* TXE */)); // Wait for space in TX buffer (TX count 1.1→1, RX count 0→0.1, should be quite fast)
	SPI1_DR = value; // Load third byte to send (TX count 1→2)
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for second inbound byte to be received (TX count 2→1.1, RX count 0.1→1)
	(void) SPI1_DR; // Discard second received byte (RX count 1→0)
	while (!(SPI1_SR & (1 << 0) /* RXNE */)); // Wait for third inbound byte to be received (TX count 1.1→0, RX count 0→1)
	(void) SPI1_DR; // Discard third received byte (RX count 1→0)
	while (SPI1_SR & (1 << 7) /* BSY */); // Wait for bus to be fully idle
	sleep_50ns();
	GPIOA_ODR |= 1 << 15; // PA15 = MRF /CS = 1; deassert chip select
}

