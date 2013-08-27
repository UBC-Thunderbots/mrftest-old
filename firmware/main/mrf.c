#include "mrf.h"
#include "globals.h"
#include "io.h"
#include "sleep.h"
#include "syscalls.h"
#include <inttypes.h>
#include <stdio.h>

typedef enum {
	MRF_REG_SHORT_RXMCR,
	MRF_REG_SHORT_PANIDL,
	MRF_REG_SHORT_PANIDH,
	MRF_REG_SHORT_SADRL,
	MRF_REG_SHORT_SADRH,
	MRF_REG_SHORT_EADR0,
	MRF_REG_SHORT_EADR1,
	MRF_REG_SHORT_EADR2,
	MRF_REG_SHORT_EADR3,
	MRF_REG_SHORT_EADR4,
	MRF_REG_SHORT_EADR5,
	MRF_REG_SHORT_EADR6,
	MRF_REG_SHORT_EADR7,
	MRF_REG_SHORT_RXFLUSH,
	MRF_REG_SHORT_ORDER = 0x10,
	MRF_REG_SHORT_TXMCR,
	MRF_REG_SHORT_ACKTMOUT,
	MRF_REG_SHORT_ESLOTG1,
	MRF_REG_SHORT_SYMTICKL,
	MRF_REG_SHORT_SYMTICKH,
	MRF_REG_SHORT_PACON0,
	MRF_REG_SHORT_PACON1,
	MRF_REG_SHORT_PACON2,
	MRF_REG_SHORT_TXBCON0 = 0x1A,
	MRF_REG_SHORT_TXNCON,
	MRF_REG_SHORT_TXG1CON,
	MRF_REG_SHORT_TXG2CON,
	MRF_REG_SHORT_ESLOTG23,
	MRF_REG_SHORT_ESLOTG45,
	MRF_REG_SHORT_ESLOTG67,
	MRF_REG_SHORT_TXPEND,
	MRF_REG_SHORT_WAKECON,
	MRF_REG_SHORT_FRMOFFSET,
	MRF_REG_SHORT_TXSTAT,
	MRF_REG_SHORT_TXBCON1,
	MRF_REG_SHORT_GATECLK,
	MRF_REG_SHORT_TXTIME,
	MRF_REG_SHORT_HSYMTMRL,
	MRF_REG_SHORT_HSYMTMRH,
	MRF_REG_SHORT_SOFTRST,
	MRF_REG_SHORT_SECCON0 = 0x2C,
	MRF_REG_SHORT_SECCON1,
	MRF_REG_SHORT_TXSTBL,
	MRF_REG_SHORT_RXSR = 0x30,
	MRF_REG_SHORT_INTSTAT,
	MRF_REG_SHORT_INTCON,
	MRF_REG_SHORT_GPIO,
	MRF_REG_SHORT_TRISGPIO,
	MRF_REG_SHORT_SLPACK,
	MRF_REG_SHORT_RFCTL,
	MRF_REG_SHORT_SECCR2,
	MRF_REG_SHORT_BBREG0,
	MRF_REG_SHORT_BBREG1,
	MRF_REG_SHORT_BBREG2,
	MRF_REG_SHORT_BBREG3,
	MRF_REG_SHORT_BBREG4,
	MRF_REG_SHORT_BBREG6 = 0x3E,
	MRF_REG_SHORT_CCAEDTH,
} mrf_reg_short_t;

typedef enum {
	MRF_REG_LONG_TXNFIFO = 0x000,
	MRF_REG_LONG_TXBFIFO = 0x080,
	MRF_REG_LONG_TXGTS1FIFO = 0x100,
	MRF_REG_LONG_TXGTS2FIFO = 0x180,
	MRF_REG_LONG_RFCON0 = 0x200,
	MRF_REG_LONG_RFCON1,
	MRF_REG_LONG_RFCON2,
	MRF_REG_LONG_RFCON3,
	MRF_REG_LONG_RFCON5 = 0x205,
	MRF_REG_LONG_RFCON6,
	MRF_REG_LONG_RFCON7,
	MRF_REG_LONG_RFCON8,
	MRF_REG_LONG_SLPCAL0,
	MRF_REG_LONG_SLPCAL1,
	MRF_REG_LONG_SLPCAL2,
	MRF_REG_LONG_RFSTATE = 0x20F,
	MRF_REG_LONG_RSSI,
	MRF_REG_LONG_SLPCON0,
	MRF_REG_LONG_SLPCON1 = 0x220,
	MRF_REG_LONG_WAKETIMEL = 0x222,
	MRF_REG_LONG_WAKETIMEH,
	MRF_REG_LONG_REMCNTL,
	MRF_REG_LONG_REMCNTH,
	MRF_REG_LONG_MAINCNT0,
	MRF_REG_LONG_MAINCNT1,
	MRF_REG_LONG_MAINCNT2,
	MRF_REG_LONG_MAINCNT3,
	MRF_REG_LONG_TESTMODE = 0x22F,
	MRF_REG_LONG_ASSOEADR0,
	MRF_REG_LONG_ASSOEADR1,
	MRF_REG_LONG_ASSOEADR2,
	MRF_REG_LONG_ASSOEADR3,
	MRF_REG_LONG_ASSOEADR4,
	MRF_REG_LONG_ASSOEADR5,
	MRF_REG_LONG_ASSOEADR6,
	MRF_REG_LONG_ASSOEADR7,
	MRF_REG_LONG_ASSOSADR0,
	MRF_REG_LONG_ASSOSADR1,
	MRF_REG_LONG_UPNONCE0 = 0x240,
	MRF_REG_LONG_UPNONCE1,
	MRF_REG_LONG_UPNONCE2,
	MRF_REG_LONG_UPNONCE3,
	MRF_REG_LONG_UPNONCE4,
	MRF_REG_LONG_UPNONCE5,
	MRF_REG_LONG_UPNONCE6,
	MRF_REG_LONG_UPNONCE7,
	MRF_REG_LONG_UPNONCE8,
	MRF_REG_LONG_UPNONCE9,
	MRF_REG_LONG_UPNONCE10,
	MRF_REG_LONG_UPNONCE11,
	MRF_REG_LONG_UPNONCE12,
	MRF_REG_LONG_KEYFIFO = 0x280,
	MRF_REG_LONG_RXFIFO = 0x300,
} mrf_reg_long_t;

typedef enum {
	// The receiver is doing nothing.
	RX_STATE_IDLE,
	// The receiver is running a DMA transfer to bring in a packet from the radio.
	RX_STATE_COPYING,
} rx_state_t;

typedef enum {
	// The transmitter is doing nothing.
	TX_STATE_IDLE,
	// The transmitter is running a DMA transfer to copy a packet from the buffer to the radio.
	TX_STATE_COPYING,
	// The transmitter is pushing a packet in the radio out over the air.
	TX_STATE_SENDING,
} tx_state_t;

uint8_t mrf_rx_buffer[128], mrf_tx_buffer[128];
static rx_state_t rx_state = RX_STATE_IDLE;
static tx_state_t tx_state = TX_STATE_IDLE;
static bool rx_packet_pending = false, tx_done_pending = false;
static bool tx_reliable, tx_successful;

static void reset(void) {
	IO_MRF.csr.reset = true;
}

static void release_reset(void) {
	IO_MRF.csr.reset = false;
}

static bool get_interrupt(void) {
	return IO_MRF.csr.interrupt;
}

static uint8_t read_short(uint8_t reg) {
	IO_MRF.address = reg;

	io_mrf_csr_t csr = { 0 };
	csr.spi_active = 1;
	IO_MRF.csr = csr;
	while (IO_MRF.csr.spi_active);
	return IO_MRF.data;
}

static void write_short(uint8_t reg, uint8_t value) {
	IO_MRF.address = reg;
	IO_MRF.data = value;

	io_mrf_csr_t csr = { 0 };
	csr.write = 1;
	csr.spi_active = 1;
	IO_MRF.csr = csr;
	while (IO_MRF.csr.spi_active);
}

static uint8_t read_long(uint16_t reg) {
	IO_MRF.address = reg;

	io_mrf_csr_t csr = { 0 };
	csr.long_address = 1;
	csr.spi_active = 1;
	IO_MRF.csr = csr;
	while (IO_MRF.csr.spi_active);
	return IO_MRF.data;
}

static void write_long(uint16_t reg, uint8_t value) {
	IO_MRF.address = reg;
	IO_MRF.data = value;

	io_mrf_csr_t csr = { 0 };
	csr.write = 1;
	csr.long_address = 1;
	csr.spi_active = 1;
	IO_MRF.csr = csr;
	while (IO_MRF.csr.spi_active);
}

void mrf_init(uint8_t channel, bool symbol_rate, uint16_t pan_id, uint16_t short_address, uint64_t mac_address) {
	// Reset the chip.
	reset();
	sleep_short();
	release_reset();
	sleep_short();

	// Write a pile of fixed register values.
	static const struct init_elt_long {
		uint16_t address;
		uint8_t data;
	} INIT_ELTS[] = {
		{ MRF_REG_SHORT_SOFTRST, 0x07 },
		{ MRF_REG_SHORT_PACON2, 0x98 },
		{ MRF_REG_SHORT_TXPEND, 0x7C },
		{ MRF_REG_SHORT_TXTIME, 0x38 },
		{ MRF_REG_SHORT_TXSTBL, 0x95 },
		{ MRF_REG_LONG_RFCON0, 0x03 },
		{ MRF_REG_LONG_RFCON1, 0x02 },
		{ MRF_REG_LONG_RFCON2, 0x80 },
		{ MRF_REG_LONG_RFCON6, 0x90 },
		{ MRF_REG_LONG_RFCON7, 0x80 },
		{ MRF_REG_LONG_RFCON8, 0x10 },
		{ MRF_REG_LONG_SLPCON0, 0x03 },
		{ MRF_REG_LONG_SLPCON1, 0x21 },
		{ MRF_REG_SHORT_RXFLUSH, 0x61 },
		{ MRF_REG_SHORT_BBREG2, 0xB8 },
		// Default threshold for bare chip is 0x60 = -69dB
		// MRF24J40MB LNA has 20dB gain
		// 0xC6 = -49 dB at the chip, corresponding to -69 on the air
		{ MRF_REG_SHORT_CCAEDTH, 0xC6 },
		{ MRF_REG_SHORT_BBREG6, 0x40 },
	};
	for (uint8_t i = 0; i < sizeof(INIT_ELTS) / sizeof(*INIT_ELTS); ++i) {
		if (INIT_ELTS[i].address >= 0x200) {
			write_long(INIT_ELTS[i].address, INIT_ELTS[i].data);
		} else {
			write_short(INIT_ELTS[i].address, INIT_ELTS[i].data);
		}
	}

	// Wait for interrupt line to be low, as we have just switched its active polarity.
	while (get_interrupt());

	// Initialize per-configuration stuff.
	write_long(MRF_REG_LONG_RFCON0, ((channel - 0x0B) << 4) | 0x03);
	write_long(MRF_REG_LONG_RFCON3, 0x18);
	write_short(MRF_REG_SHORT_RFCTL, 0x04);
	write_short(MRF_REG_SHORT_RFCTL, 0x00);
	sleep_short();
	if (symbol_rate) {
		write_short(MRF_REG_SHORT_BBREG0, 0x01);
		write_short(MRF_REG_SHORT_BBREG3, 0x34);
		write_short(MRF_REG_SHORT_BBREG4, 0x5C);
		write_short(MRF_REG_SHORT_SOFTRST, 0x02);
	}
	write_short(MRF_REG_SHORT_PANIDL, pan_id);
	write_short(MRF_REG_SHORT_PANIDH, pan_id >> 8);
	for (uint8_t i = 0; i < 8; ++i) {
		write_short(MRF_REG_SHORT_EADR0 + i, mac_address);
		mac_address >>= 8;
	}
	write_short(MRF_REG_SHORT_SADRH, short_address >> 8);
	write_short(MRF_REG_SHORT_SADRL, short_address);

	// Enable the external amplifiers.
	// For MRF24J40MB, no separate power regulator control is needed, but GPIO2:1 need to be configured to control the amplifiers and RF switches.
	write_short(MRF_REG_SHORT_GPIO, 0x00);
	write_long(MRF_REG_LONG_TESTMODE, 0x0F);
	write_short(MRF_REG_SHORT_TRISGPIO, 0x3F);

	// Enable interrupts on receive and transmit complete.
	write_short(MRF_REG_SHORT_INTCON, 0b11110110);
}

static void poll_interrupts(void) {
	if (get_interrupt()) {
		unsigned int start = IO_SYSCTL.tsc;
		uint8_t intstat = read_short(MRF_REG_SHORT_INTSTAT);
		if (intstat & 0x01) {
			tx_done_pending = true;
		}
		if (intstat & 0x08) {
			rx_packet_pending = true;
		}
		cpu_usage += IO_SYSCTL.tsc - start;
	}
}

static void check_dma_error(void) {
	if (IO_MRF.csr.dma_error) {
		// Report the problem.
#define MESSAGE "MRF DMA error at address %p for register %u!\n"
		char buffer[sizeof(MESSAGE) + 20];
		siprintf(buffer, MESSAGE, IO_MRF.dma_address, IO_MRF.address);
#undef MESSAGE
		syscall_debug_puts(buffer);

		// Crash!
		*((volatile unsigned int *) 0) = 27;
	}
}

bool mrf_rx_poll(void) {
	unsigned int start;

	// If the DMA engine is currently active in either direction (transmit or receive) or the transceiver is busy, it is unsafe to touch the bus, so do nothing.
	{
		io_mrf_csr_t csr = IO_MRF.csr;
		if (csr.dma_active || csr.spi_active) {
			return false;
		}
	}

	// If an error occurred, report it.
	check_dma_error();

	switch (rx_state) {
		case RX_STATE_IDLE:
			// No packet is currently outstanding.
			// Check if a packet has been received and is waiting in the radio.
			poll_interrupts();
			if (rx_packet_pending) {
				start = IO_SYSCTL.tsc;
				// Clear pending interrupt.
				rx_packet_pending = false;
				// Disable reception of further packets from the air.
				write_short(MRF_REG_SHORT_BBREG1, 0x04); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception
				// Read the length of the packet from the receive buffer’s first byte.
				uint8_t packet_length = read_long(MRF_REG_LONG_RXFIFO);
				mrf_rx_buffer[0] = packet_length;
				// Start a DMA data copy to bring the packet into the receive buffer.
				IO_MRF.address = MRF_REG_LONG_RXFIFO + 1;
				IO_MRF.dma_address = &mrf_rx_buffer[1];
				IO_MRF.dma_length = packet_length;
				io_mrf_csr_t csr = { 0 };
				csr.dma_active = 1;
				csr.long_address = 1;
				IO_MRF.csr = csr;
				// Record state change.
				rx_state = RX_STATE_COPYING;
				cpu_usage += IO_SYSCTL.tsc - start;
			}
			return false;

		case RX_STATE_COPYING:
			start = IO_SYSCTL.tsc;
			// We were using the DMA engine to copy a received packet into the receive buffer, but since we got here, the DMA engine must be idle.
			// Therefore, the packet is now finished being copied.
			// Re-enable reception of packets from the air into the radio.
			write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
			// Record state change and hand the packet to the application.
			rx_state = RX_STATE_IDLE;
			cpu_usage += IO_SYSCTL.tsc - start;
			__sync_synchronize();
			return true;
	}

	return false;
}

static void mrf_tx_poll(void) {
	unsigned int start;

	// If the DMA engine is currently active in either direction (transmit or receive) or the transceiver is busy, it is unsafe to touch the bus, so do nothing.
	{
		io_mrf_csr_t csr = IO_MRF.csr;
		if (csr.dma_active || csr.spi_active) {
			return;
		}
	}

	// If an error occurred, report it.
	check_dma_error();

	switch (tx_state) {
		case TX_STATE_IDLE:
			// There is nothing to do here until the application asks us to act.
			return;

		case TX_STATE_COPYING:
			// We were using the DMA engine to copy a new packet into the transmit buffer, but since we got here, the DMA engine must be idle.
			// Therefore, the packet is now finished being copied.
			start = IO_SYSCTL.tsc;
			// Start the radio transmitting the packet.
			write_short(MRF_REG_SHORT_TXNCON, 0b00000101);
			// Record state change.
			tx_state = TX_STATE_SENDING;
			cpu_usage += IO_SYSCTL.tsc - start;
			return;

		case TX_STATE_SENDING:
			// A packet was being sent over the air.
			// See if that has finished yet.
			poll_interrupts();
			if (tx_done_pending) {
				start = IO_SYSCTL.tsc;
				// Clear pending interrupt.
				tx_done_pending = false;
				// Check status of transmission.
				uint8_t txstat = read_short(MRF_REG_SHORT_TXSTAT);
				if ((txstat & 0x01) && tx_reliable) {
					// The packet was to be reliable and the transmission failed.
					// Resubmit the same packet with no change in system status.
					write_short(MRF_REG_SHORT_TXNCON, 0b00000101);
				} else {
					// Transmission succeeded or the packet was declared unreliable.
					// Either way, the transmit path is now free.
					tx_state = TX_STATE_IDLE;
					// Record the outcome.
					tx_successful = !(txstat & 0x01);
				}
				cpu_usage += IO_SYSCTL.tsc - start;
			}
			return;
	}
}

bool mrf_tx_buffer_free(void) {
	// We allow the application to write into the buffer if the transmit path is idle or transmitting.
	// Transmitting is OK because at that point, the packet has been copied into the radio and transmission is independent of the local RAM buffer.
	mrf_tx_poll();
	return tx_state != TX_STATE_COPYING;
}

bool mrf_tx_path_free(void) {
	// It is unsafe to start a transmission if the transmit path is copying or transmitting a packet, obviously.
	// It is unsafe to start a transmission if the receive path DMA engine is running, because only one direction of DMA may occur at a time.
	mrf_tx_poll();
	io_mrf_csr_t csr = IO_MRF.csr;
	return tx_state == TX_STATE_IDLE && !csr.dma_active && !csr.spi_active;
}

void mrf_tx_start(bool reliable) {
	unsigned int start = IO_SYSCTL.tsc;

	// Record reliability flag.
	tx_reliable = reliable;

	// Start a DMA transfer to copy the packet buffer into the radio transmit buffer.
	IO_MRF.address = MRF_REG_LONG_TXNFIFO;
	IO_MRF.dma_address = mrf_tx_buffer;
	IO_MRF.dma_length = mrf_tx_buffer[1] + 2;
	io_mrf_csr_t csr = { 0 };
	csr.dma_active = 1;
	csr.long_address = 1;
	csr.write = 1;
	__sync_synchronize();
	IO_MRF.csr = csr;

	// Record state.
	tx_state = TX_STATE_COPYING;
	cpu_usage += IO_SYSCTL.tsc - start;
}

bool mrf_tx_successful(void) {
	return tx_successful;
}

