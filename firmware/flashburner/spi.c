#include <gpio.h>
#include <rcc.h>
#include <registers/spi.h>
#include <sleep.h>
#include "spi.h"

void spi_init(void) {
	// Reset and enable the SPI module.
	rcc_enable_reset(APB2, SPI1);

	// Configure the SPI module for the target.
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
			.BR = 0, // Transmission speed is 84 MHz (APB2) ÷ 2 = 42 MHz.
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

	// Reset and enable the SPI module.
	rcc_enable_reset(APB1, SPI3);

	// Configure the SPI module for the onboard memory.
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
		SPI3.CR2 = tmp;
	}
	{
		SPI_CR1_t tmp = {
			.CPHA = 0, // Capture on first clock transition, drive new data on second.
			.CPOL = 0, // Clock idles low.
			.MSTR = 1, // Master mode.
			.BR = 0, // Transmission speed is 42 MHz (APB1) ÷ 2 = 21 MHz.
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
		SPI3.CR1 = tmp;
	}
}

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

static void int_drive_bus(void) {
}

static void int_assert_cs(void) {
	sleep_50ns();
	gpio_reset(GPIOA, 4);
	sleep_50ns();
}

static void int_deassert_cs(void) {
	while (SPI3.SR.BSY); // Wait until no activity
	sleep_50ns();
	gpio_set(GPIOA, 4);
}

static uint8_t int_transceive_byte(uint8_t byte) {
	while (!SPI3.SR.TXE); // Wait for send buffer space
	SPI3.DR = byte;
	while (!SPI3.SR.RXNE); // Wait for inbound byte to be received
	return SPI3.DR;
}

static void int_read_bytes(void *buffer, size_t length) {
	uint8_t *pch = buffer;
	size_t sent = 0, received = 0;
	while (sent < length || received < length) {
		if (received < length && SPI3.SR.RXNE) {
			*pch++ = SPI3.DR;
			++received;
		}
		if (sent < length && SPI3.SR.TXE) {
			SPI3.DR = 0;
			++sent;
		}
	}
}

static void int_write_bytes(const void *buffer, size_t length) {
	size_t sent = 0, received = 0;
	const uint8_t *pch = buffer;
	while (sent < length || received < length) {
		if (received < length && SPI3.SR.RXNE) {
			(void) SPI3.DR;
			++received;
		}
		if (sent < length && SPI3.SR.TXE) {
			SPI3.DR = *pch++;
			++sent;
		}
	}
}

const struct spi_ops spi_internal_ops = {
	.drive_bus = &int_drive_bus,
	.assert_cs = &int_assert_cs,
	.deassert_cs = &int_deassert_cs,
	.transceive_byte = &int_transceive_byte,
	.read_bytes = &int_read_bytes,
	.write_bytes = &int_write_bytes,
};

static void ext_drive_bus(void) {
	// Switch the MOSI (PB5), MISO (PB4), and Clock (PB3) into alternate function mode.
	gpio_set_mode(GPIOB, 3, GPIO_MODE_AF);
	gpio_set_mode(GPIOB, 4, GPIO_MODE_AF);
	gpio_set_mode(GPIOB, 5, GPIO_MODE_AF);

	// Switch /CS (PA15) to output high.
	gpio_set(GPIOA, 15);
	gpio_set_mode(GPIOA, 15, GPIO_MODE_OUT);
}

static void ext_assert_cs(void) {
	sleep_50ns();
	gpio_reset(GPIOA, 15);
	sleep_50ns();
}

static void ext_deassert_cs(void) {
	while (SPI1.SR.BSY); // Wait until no activity
	sleep_50ns();
	gpio_set(GPIOA, 15);
}

static uint8_t ext_transceive_byte(uint8_t byte) {
	while (!SPI1.SR.TXE); // Wait for send buffer space
	SPI1.DR = byte;
	while (!SPI1.SR.RXNE); // Wait for inbound byte to be received
	return SPI1.DR;
}

static void ext_read_bytes(void *buffer, size_t length) {
	uint8_t *pch = buffer;
	size_t sent = 0, received = 0;
	while (sent < length || received < length) {
		if (received < length && SPI1.SR.RXNE) {
			*pch++ = SPI1.DR;
			++received;
		}
		if (sent < length && SPI1.SR.TXE) {
			SPI1.DR = 0;
			++sent;
		}
	}
}

static void ext_write_bytes(const void *buffer, size_t length) {
	size_t sent = 0, received = 0;
	const uint8_t *pch = buffer;
	while (sent < length || received < length) {
		if (received < length && SPI1.SR.RXNE) {
			(void) SPI1.DR;
			++received;
		}
		if (sent < length && SPI1.SR.TXE) {
			SPI1.DR = *pch++;
			++sent;
		}
	}
}

const struct spi_ops spi_external_ops = {
	.drive_bus = &ext_drive_bus,
	.assert_cs = &ext_assert_cs,
	.deassert_cs = &ext_deassert_cs,
	.transceive_byte = &ext_transceive_byte,
	.read_bytes = &ext_read_bytes,
	.write_bytes = &ext_write_bytes,
};

