#include "mrf.h"
#include "dma.h"
#include "globals.h"
#include "io.h"
#include "sleep.h"
#include "tsc.h"

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
static bool tx_reliable;

static void reset(void) {
	MRF_CTL = 0x00;
}

static void release_reset(void) {
	MRF_CTL = 0x01;
}

static bool get_interrupt(void) {
	return !!(MRF_CTL & 0x04);
}

static uint8_t read_short(uint8_t reg) {
	MRF_ADDR = reg;
	MRF_CTL = 0b00001001;
	while (MRF_CTL & 0x80);
	return MRF_DATA;
}

static void write_short(uint8_t reg, uint8_t value) {
	MRF_ADDR = reg;
	MRF_DATA = value;
	MRF_CTL = 0b00100001;
	while (MRF_CTL & 0x80);
}

static uint8_t read_long(uint16_t reg) {
	MRF_ADDR = reg >> 8;
	MRF_ADDR = reg;
	MRF_CTL = 0b00010001;
	while (MRF_CTL & 0x80);
	return MRF_DATA;
}

static void write_long(uint16_t reg, uint8_t value) {
	MRF_ADDR = reg >> 8;
	MRF_ADDR = reg;
	MRF_DATA = value;
	MRF_CTL = 0b01000001;
	while (MRF_CTL & 0x80);
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
		{ MRF_REG_SHORT_BBREG2, 0x80 },
		{ MRF_REG_SHORT_CCAEDTH, 0x60 },
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
		uint32_t start = rdtsc();
		uint8_t intstat = read_short(MRF_REG_SHORT_INTSTAT);
		if (intstat & 0x01) {
			tx_done_pending = true;
		}
		if (intstat & 0x08) {
			rx_packet_pending = true;
		}
		cpu_usage += rdtsc() - start;
	}
}

bool mrf_rx_poll(void) {
	uint32_t start;

	// If the DMA engine is currently active in either direction (transmit or receive) or the transceiver is busy, it is unsafe to touch the bus, so do nothing.
	if (dma_running(DMA_READ_CHANNEL_MRF) || dma_running(DMA_WRITE_CHANNEL_MRF) || (MRF_CTL & 0x80)) {
		return false;
	}

	switch (rx_state) {
		case RX_STATE_IDLE:
			// No packet is currently outstanding.
			// Check if a packet has been received and is waiting in the radio.
			poll_interrupts();
			if (rx_packet_pending) {
				start = rdtsc();
				// Clear pending interrupt.
				rx_packet_pending = false;
				// Disable reception of further packets from the air.
				write_short(MRF_REG_SHORT_BBREG1, 0x04); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception
				// Read the length of the packet from the receive buffer’s first byte.
				uint8_t packet_length = read_long(MRF_REG_LONG_RXFIFO);
				mrf_rx_buffer[0] = packet_length;
				// Start a DMA data copy to bring the packet into the receive buffer.
				MRF_ADDR = (MRF_REG_LONG_RXFIFO + 1) >> 8;
				MRF_ADDR = (uint8_t) (MRF_REG_LONG_RXFIFO + 1);
				dma_write_start(DMA_WRITE_CHANNEL_MRF, mrf_rx_buffer + 1, packet_length);
				// Record state change.
				rx_state = RX_STATE_COPYING;
				cpu_usage += rdtsc() - start;
			}
			return false;

		case RX_STATE_COPYING:
			start = rdtsc();
			// We were using the DMA engine to copy a received packet into the receive buffer, but since we got here, the DMA engine must be idle.
			// Therefore, the packet is now finished being copied.
			// Re-enable reception of packets from the air into the radio.
			write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
			// Record state change and hand the packet to the application.
			rx_state = RX_STATE_IDLE;
			cpu_usage += rdtsc() - start;
			return true;
	}

	return false;
}

static void mrf_tx_poll(void) {
	uint32_t start;

	// If the DMA engine is currently active in either direction (transmit or receive) or the transceiver is busy, it is unsafe to touch the bus, so do nothing.
	if (dma_running(DMA_READ_CHANNEL_MRF) || dma_running(DMA_WRITE_CHANNEL_MRF) || (MRF_CTL & 0x80)) {
		return;
	}

	switch (tx_state) {
		case TX_STATE_IDLE:
			// There is nothing to do here until the application asks us to act.
			return;

		case TX_STATE_COPYING:
			// We were using the DMA engine to copy a new packet into the transmit buffer, but since we got here, the DMA engine must be idle.
			// Therefore, the packet is now finished being copied.
			start = rdtsc();
			// Start the radio transmitting the packet.
			write_short(MRF_REG_SHORT_TXNCON, 0b00000101);
			// Record state change.
			tx_state = TX_STATE_SENDING;
			cpu_usage += rdtsc() - start;
			return;

		case TX_STATE_SENDING:
			// A packet was being sent over the air.
			// See if that has finished yet.
			poll_interrupts();
			if (tx_done_pending) {
				start = rdtsc();
				// Clear pending interrupt.
				tx_done_pending = false;
				// Check status of transmission.
				uint8_t txstat = read_short(MRF_REG_SHORT_TXSTAT);
				if ((txstat & 0x01) && tx_reliable) {
					// The packet was to be reliable and the transmission failed.
					// Resubmit the same packet with no change in system status.
					write_short(MRF_REG_SHORT_TXNCON, 0b00000101);
				} else {
					// Transmission succeeded or the packet was declared unreliable so nobody cares.
					// Either way, the transmit path is now free.
					tx_state = TX_STATE_IDLE;
				}
				cpu_usage += rdtsc() - start;
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
	// It is safe to start a transmission if the transceiver is busy because the DMA engine will wait its turn behind the previous bus transaction.
	mrf_tx_poll();
	return tx_state == TX_STATE_IDLE && !dma_running(DMA_WRITE_CHANNEL_MRF);
}

void mrf_tx_start(bool reliable) {
	uint32_t start = rdtsc();

	// Record reliability flag.
	tx_reliable = reliable;

	// Start a DMA transfer to copy the packet buffer into the radio transmit buffer.
	MRF_ADDR = MRF_REG_LONG_TXNFIFO >> 8;
	MRF_ADDR = MRF_REG_LONG_TXNFIFO;
	dma_read_start(DMA_READ_CHANNEL_MRF, mrf_tx_buffer, mrf_tx_buffer[1] + 2);

	// Record state.
	tx_state = TX_STATE_COPYING;

	cpu_usage += rdtsc() - start;
}

