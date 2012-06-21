#include "config.h"
#include "configs.h"
#include "interrupt.h"
#include "mrf.h"
#include "perconfig.h"
#include "rcc.h"
#include "registers.h"
#include "sleep.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"
#include "usb.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"

const uint8_t CONFIGURATION_DESCRIPTOR2[] = {
	9, // bLength
	2, // bDescriptorType
	60, // wTotalLength LSB
	0, // wTotalLength MSB
	1, // bNumInterfaces
	2, // bConfigurationValue
	4, // iConfiguration
	0x80, // bmAttributes
	150, // bMaxPower

	9, // bLength
	4, // bDescriptorType
	0, // bInterfaceNumber
	0, // bAlternateSetting
	6, // bNumEndpoints
	0xFF, // bInterfaceClass
	0x00, // bInterfaceSubClass
	0, // bInterfaceProtocol
	0, // iInterface

	7, // bLength
	5, // bDescriptorType
	0x01, // bEndpointAddress
	0x03, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	5, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x02, // bEndpointAddress
	0x03, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	5, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x03, // bEndpointAddress
	0x03, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	5, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x81, // bEndpointAddress
	0x03, // bmAttributes
	2, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	5, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x82, // bEndpointAddress
	0x03, // bmAttributes
	64, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	5, // bInterval

	7, // bLength
	5, // bDescriptorType
	0x83, // bEndpointAddress
	0x03, // bmAttributes
	2, // wMaxPacketSize LSB
	0, // wMaxPacketSize MSB
	5, // bInterval
};

#define PKT_FLAG_RELIABLE 0x01

static bool in_ep1_enabled, in_ep2_enabled, in_ep2_zlp_pending, out_ep1_enabled, out_ep2_enabled, out_ep3_enabled, drive_packet_pending, mrf_tx_active;
static uint8_t mrf_tx_seqnum;
static uint16_t mrf_rx_seqnum[8];
static normal_out_packet_t *unreliable_out_free, *reliable_out_free, *first_out_pending, *last_out_pending, *cur_out_transmitting, *first_mdr_pending, *last_mdr_pending;
static normal_in_packet_t *in_free, *first_in_pending, *last_in_pending;
static unsigned int poll_index;

static void send_drive_packet(void) {
	// Write out the packet
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 0, 9); // Header length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 1, 9 + sizeof(perconfig.normal.drive_packet)); // Frame length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 2, 0b01000001); // Frame control LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 3, 0b10001000); // Frame control MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 4, ++mrf_tx_seqnum); // Sequence number
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 5, config.pan_id); // Destination PAN ID LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 6, config.pan_id >> 8); // Destination PAN ID MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 7, 0xFF); // Destination address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 8, 0xFF); // Destination address MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 9, 0x00); // Source address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 10, 0x01); // Source address MSB
	const uint8_t *bptr = (const uint8_t *) perconfig.normal.drive_packet;
	for (size_t i = 0; i < sizeof(perconfig.normal.drive_packet); ++i) {
		uint8_t mask = 0;
		if (i - 1 == poll_index * sizeof(perconfig.normal.drive_packet) / 8) {
			mask = 0x80;
		}
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i, bptr[i] | mask);
	}
	poll_index = (poll_index + 1) % 8;

	// Initiate transmission with no acknowledgement
	mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000001);
	mrf_tx_active = true;
	drive_packet_pending = false;

	// Blink the transmit light
	GPIOB_ODR ^= 2 << 12;
}

void timer6_interrupt_vector(void) {
	// Clear interrupt flag
	TIM6_SR = 0;

	// Check if a packet is currently being transmitted
	if (!mrf_tx_active) {
		send_drive_packet();
	} else {
		drive_packet_pending = true;
	}
}

static void push_mrf_tx(void) {
	// Don’t try to transmit if already doing so
	if (mrf_tx_active) {
		return;
	}

	// Decide what type of packet we should send
	if (!cur_out_transmitting && drive_packet_pending) {
		// A timer interrupt indicated we should send a drive packet, but the transmitter was active; send it now
		send_drive_packet();
	} else if (cur_out_transmitting || first_out_pending) {
		// No need to send a drive packet right now, but there is an ordinary data packet waiting; send that
		// Pop the packet from the queue, if there is no current packet
		normal_out_packet_t *pkt;
		if (cur_out_transmitting) {
			pkt = cur_out_transmitting;
		} else {
			pkt = first_out_pending;
			first_out_pending = pkt->next;
			if (!first_out_pending) {
				last_out_pending = 0;
			}
			pkt->next = 0;
			cur_out_transmitting = pkt;
			++mrf_tx_seqnum;
		}

		// Write the packet into the transmit buffer
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 0, 9); // Header length
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 1, 9 + pkt->length); // Frame length
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 2, 0b01100001); // Frame control LSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 3, 0b10001000); // Frame control MSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 4, mrf_tx_seqnum); // Sequence number
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 5, config.pan_id); // Destination PAN ID LSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 6, config.pan_id >> 8); // Destination PAN ID MSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 7, pkt->dest); // Destination address LSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 8, 0x00); // Destination address MSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 9, 0x00); // Source address LSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 10, 0x01); // Source address MSB
		for (size_t i = 0; i < pkt->length; ++i) {
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i, pkt->data[i]);
		}

		// Initiate transmission with acknowledgement
		mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);
		mrf_tx_active = true;
	}
}

static void push_mdrs(void) {
	// If the message delivery endpoint already has data queued, don’t try to send more data
	if (in_ep1_enabled) {
		return;
	}

	// If there are no MDRs waiting, do nothing
	if (!first_mdr_pending) {
		return;
	}

	// Pop an MDR from the queue
	normal_out_packet_t *pkt = first_mdr_pending;
	first_mdr_pending = pkt->next;
	if (!first_mdr_pending) {
		last_mdr_pending = 0;
	}
	pkt->next = 0;

	// Push the delivery status into the USB FIFO
	OTG_FS_DIEPTSIZ1 = 
		(1 << 19) // PKTCNT = 1; send one packet
		| (2 << 0); // XFRSIZ = 2; send two bytes
	OTG_FS_DIEPCTL1 |=
		(1 << 31) // EPENA = 1; enable endpoint
		| (1 << 26); // CNAK = 1; clear NAK flag
	OTG_FS_FIFO[1][0] = pkt->message_id | (pkt->delivery_status << 8);

	// Push the consumed MDR packet buffer onto the free stack
	pkt->next = reliable_out_free;
	reliable_out_free = pkt;
}

static void push_rx(void) {
	// If a received message endpoint already has data queued, don’t try to send more data
	if (in_ep2_enabled) {
		return;
	}

	// Check if a prior transfer needed a zero-length packet
	if (in_ep2_zlp_pending) {
		// A zero length packet was needed; do that now
		OTG_FS_DIEPTSIZ2 =
			(1 << 19) // PKTCNT = 1; send one packet
			| (0 << 0); // XFRSIZ = 0; send zero bytes
		OTG_FS_DIEPCTL2 |=
			(1 << 31) // EPENA = 1; enable endpoint
			| (1 << 26); // CNAK = 1; clear NAK flag
		in_ep2_zlp_pending = false;
		in_ep2_enabled = true;
		return;
	}

	// If there are no received messages waiting to be sent, do nothing
	if (!first_in_pending) {
		return;
	}

	// Pop a packet from the queue
	normal_in_packet_t *packet = first_in_pending;
	first_in_pending = packet->next;
	if (!first_in_pending) {
		last_in_pending = 0;
	}
	packet->next = 0;

	// Push this received packet into the USB FIFO
	OTG_FS_DIEPTSIZ2 =
		(((packet->length + 63) / 64) << 19) // PKTCNT = n; send the proper number of packets (this does not include any ZLP, which must be in a separate “transfer” as far as the STM32F4’s USB engine is concerned)
		| (packet->length << 0); // XFRSIZ = n; send the proper number of bytes
	OTG_FS_DIEPCTL2 |=
		(1 << 31) // EPENA = 1; enable endpoint
		| (1 << 26); // CNAK = 1; clear NAK flag
	for (size_t i = 0; i < (packet->length + 3U) / 4U; ++i) {
		uint32_t word = packet->data[i * 4] | (packet->data[i * 4 + 1] << 8) | (packet->data[i * 4 + 2] << 16) | (packet->data[i * 4 + 3] << 24);
		OTG_FS_FIFO[2][0] = word;
	}
	in_ep2_enabled = true;

	// Decide if a zero-length packet will be needed after this
	in_ep2_zlp_pending = !(packet->length % 64);

	// Push this consumed packet buffer into the free list
	packet->next = in_free;
	in_free = packet;

	// Re-enable the receiver; it may have been left disabled if there were no free packet buffers for flow control purposes
	mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
}

static void exti12_interrupt_vector(void) {
	// Clear the interrupt
	EXTI_PR = 1 << 12; // PR12 = 1; clear pending EXTI12 interrupt

	while (GPIOC_IDR & (1 << 12) /* PC12 */) {
		// Check outstanding interrupts
		uint8_t intstat = mrf_read_short(MRF_REG_SHORT_INTSTAT);
		if (intstat & (1 << 3)) {
			// RXIF = 1; packet received
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception
			// Read out the frame length byte and frame control word
			uint8_t rxfifo_frame_length = mrf_read_long(MRF_REG_LONG_RXFIFO);
			uint16_t frame_control = mrf_read_long(MRF_REG_LONG_RXFIFO + 1) | (mrf_read_long(MRF_REG_LONG_RXFIFO + 2) << 8);
			// Sanity-check the frame control word
			if (((frame_control >> 0) & 7) == 1 /* Data packet */ && ((frame_control >> 3) & 1) == 0 /* No security */ && ((frame_control >> 6) & 1) == 1 /* Intra-PAN */ && ((frame_control >> 10) & 3) == 2 /* 16-bit destination address */ && ((frame_control >> 14) & 3) == 2 /* 16-bit source address */) {
				// Read out and check the source address and sequence number
				uint16_t source_address = mrf_read_long(MRF_REG_LONG_RXFIFO + 8) | (mrf_read_long(MRF_REG_LONG_RXFIFO + 9) << 8);
				uint8_t sequence_number = mrf_read_long(MRF_REG_LONG_RXFIFO + 3);
				if (source_address < 8 && sequence_number != mrf_rx_seqnum[source_address]) {
					// Blink the receive light
					GPIOB_ODR ^= 4 << 12;

					static const uint8_t HEADER_LENGTH = 2 /* Frame control */ + 1 /* Seq# */ + 2 /* Dest PAN */ + 2 /* Dest */ + 2 /* Src */;
					static const uint8_t FOOTER_LENGTH = 2;

					// Update sequence number
					mrf_rx_seqnum[source_address] = sequence_number;

					// Allocate a packet buffer
					normal_in_packet_t *packet = in_free;
					in_free = packet->next;
					packet->next = 0;

					// Fill in the packet buffer
					packet->length = 1 + rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH;
					packet->data[0] = source_address;
					for (uint8_t i = 0; i < rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH; ++i) {
						packet->data[i + 1] = mrf_read_long(MRF_REG_LONG_RXFIFO + 1 + HEADER_LENGTH + i);
					}

					// Push the packet on the receive queue
					if (last_in_pending) {
						last_in_pending->next = packet;
						last_in_pending = packet;
					} else {
						first_in_pending = last_in_pending = packet;
					}

					// Kick the receive queue
					push_rx();
				}
			}

			// Only re-enable the receiver if we have a free packet buffer into which to receive additional data
			// Not re-enabling the receiver if we don’t have a packet buffer will cause the MRF not to ACK,
			// which serves as flow control and guarantees that every ACKed packet is, in fact, delivered
			if (in_free) {
				mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
			}
		}
		if (intstat & (1 << 0)) {
			// TXIF = 1; transmission complete (successful or failed)
			// Read the transmission status to determine the packet’s fate
			normal_out_packet_t *pkt = cur_out_transmitting;
			cur_out_transmitting = 0;
			uint8_t txstat = mrf_read_short(MRF_REG_SHORT_TXSTAT);
			if (txstat & 0x01) {
				// Transmission failed
				if (txstat & (1 << 5)) {
					// CCA failed
					pkt->delivery_status = 0x03;
				} else {
					// Assume: No ACK
#warning possibly bad assumption; add a code for "unknown reason"
					pkt->delivery_status = 0x02;
				}
			} else {
				// Transmission successful
				pkt->delivery_status = 0x00;
			}

			// Decide whether to try transmission again or to retire the packet
			if (pkt->delivery_status == 0x00 || !pkt->retries_remaining) {
				// Push the packet buffer onto the message delivery report queue (if reliable, else free)
				if (pkt->reliable) {
					// Save the message delivery report
					if (last_mdr_pending) {
						last_mdr_pending->next = pkt;
						last_mdr_pending = pkt;
					} else {
						first_mdr_pending = last_mdr_pending = pkt;
					}

					// Kick the MDR queue
					push_mdrs();
				} else {
					pkt->next = unreliable_out_free;
					unreliable_out_free = pkt;
				}
			} else {
				cur_out_transmitting = pkt;
				--pkt->retries_remaining;
			}

			// The transmitter is no longer active
			mrf_tx_active = false;

			// Kick the transmit queue
			push_mrf_tx();
		}
	}
}

static void on_ep1_in_interrupt(void) {
	if (OTG_FS_DIEPINT1 & (1 << 0) /* XFRC */) {
		// On transfer complete, go see if we have another MDR to send
		OTG_FS_DIEPINT1 = 1 << 0; // XFRC = 1; clear transfer complete interrupt flag
		in_ep1_enabled = false;
		push_mdrs();
	}
}

static void on_ep2_in_interrupt(void) {
	if (OTG_FS_DIEPINT2 & (1 << 0) /* XFRC */) {
		// On transfer complete, go see if we have another message to send
		OTG_FS_DIEPINT2 = 1 << 0; // XFRC = 1; clear transfer complete interrupt flag
		in_ep2_enabled = false;
		push_rx();
	}
}

static void init_out_ep1(void) {
	if (!out_ep1_enabled) {
		OTG_FS_DOEPTSIZ1 =
			(1 << 19) // PKTCNT = 1; receive one packet
			| (64 << 0); // XFRSIZ = 64; receive a packet of up to 64 bytes
		OTG_FS_DOEPCTL1 |=
			(1 << 31) // EPENA = 1; enable endpoint
			| (1 << 26); // CNAK = 1; clear NAK flag
		out_ep1_enabled = true;
	}
}

static void init_out_ep2(void) {
#warning this endpoint really ought to handle transfers of more than 64 bytes
	if (!out_ep2_enabled) {
		OTG_FS_DOEPTSIZ2 =
			(1 << 19) // PKTCNT = 1; receive one packet
			| (64 << 0); // XFRSIZ = 64; receive a packet of up to 64 bytes
		OTG_FS_DOEPCTL2 |=
			(1 << 31) // EPENA = 1; enable endpoint
			| (1 << 26); // CNAK = 1; clear NAK flag
		out_ep2_enabled = true;
	}
}

static void init_out_ep3(void) {
#warning this endpoint really ought to handle transfers of more than 64 bytes
	if (!out_ep3_enabled) {
		OTG_FS_DOEPTSIZ3 =
			(1 << 19) // PKTCNT = 1; receive one packet
			| (64 << 0); // XFRSIZ = 64; receive a packet of up to 64 bytes
		OTG_FS_DOEPCTL3 |=
			(1 << 31) // EPENA = 1; enable endpoint
			| (1 << 26); // CNAK = 1; clear NAK flag
		out_ep3_enabled = true;
	}
}

static void on_ep1_out_pattern(uint32_t pattern) {
	uint32_t pktsts = (pattern >> 17) & 0xF;
	if (pktsts == 0b0010) {
		// OUT data packet received
		uint32_t bcnt = (pattern >> 4) & 0x7FF;
		if (bcnt == sizeof(perconfig.normal.drive_packet)) {
			// Copy the received data into the drive packet buffer
			for (size_t i = 0; i < sizeof(perconfig.normal.drive_packet) / sizeof(*perconfig.normal.drive_packet); ++i) {
				perconfig.normal.drive_packet[i] = OTG_FS_FIFO[0][0];
			}
			// Enable the timer that generates drive packets on the radio
			TIM6_CR1 |= 1 << 0; // CEN = 1; enable counter now
		} else {
			for (size_t i = 0; i < bcnt; i += 4) {
				(void) OTG_FS_FIFO[0][0];
			}
		}
	} else if (pktsts == 0b0011) {
		// OUT transfer complete
		out_ep1_enabled = false;
		init_out_ep1();
	}
}

static void on_ep2_out_pattern(uint32_t pattern) {
	uint32_t pktsts = (pattern >> 17) & 0xF;
	if (pktsts == 0b0010) {
		// OUT data packet received
		uint32_t bcnt = (pattern >> 4) & 0x7FF;
		if (bcnt >= 2) {
			// Allocate a packet buffer
			normal_out_packet_t *pkt = reliable_out_free;
			reliable_out_free = pkt->next;
			pkt->next = 0;

			// Flag the packet as reliable
			pkt->reliable = true;

			// The transfer has a two-byte header
			pkt->length = bcnt - 2;

			// Allow up to twenty transport-layer retries
			pkt->retries_remaining = 20;

			// Copy out the header and first two bytes of data
			{
				uint32_t word = OTG_FS_FIFO[0][0];
				pkt->dest = word & 0x0F;
				pkt->message_id = word >> 8;
				pkt->data[0] = word >> 16;
				pkt->data[1] = word >> 24;
			}

			// Copy out the rest of the data
			for (size_t i = 2; i < bcnt - 2; i += 4) {
				uint32_t word = OTG_FS_FIFO[0][0];
				pkt->data[i] = word;
				pkt->data[i + 1] = word >> 8;
				pkt->data[i + 2] = word >> 16;
				pkt->data[i + 3] = word >> 24;
			}

			// Push the packet onto the transmit queue
			if (last_out_pending) {
				last_out_pending->next = pkt;
				last_out_pending = pkt;
			} else {
				first_out_pending = last_out_pending = pkt;
			}

			// Kick the transmit queue
			push_mrf_tx();
		}
	} else if (pktsts == 0b0011) {
		// OUT transfer complete
		out_ep2_enabled = false;
		if (reliable_out_free) {
			init_out_ep2();
		}
	}
}

static void on_ep3_out_pattern(uint32_t pattern) {
	uint32_t pktsts = (pattern >> 17) & 0xF;
	if (pktsts == 0b0010) {
		// OUT data packet received
		uint32_t bcnt = (pattern >> 4) & 0x7FF;
		if (bcnt) {
			// Allocate a packet buffer
			normal_out_packet_t *pkt = unreliable_out_free;
			unreliable_out_free = pkt->next;
			pkt->next = 0;

			// Flag the packet as unreliable
			pkt->reliable = false;

			// The transfer has a one-byte header
			pkt->length = bcnt - 1;

			// Allow up to twenty transport-layer retries
			pkt->retries_remaining = 20;

			// Copy out the header and first three bytes of data
			{
				uint32_t word = OTG_FS_FIFO[0][0];
				pkt->dest = word & 0x0F;
				pkt->data[0] = word >> 8;
				pkt->data[1] = word >> 16;
				pkt->data[2] = word >> 24;
			}

			// Copy out the rest of the data
			for (size_t i = 3; i < bcnt - 1; i += 4) {
				uint32_t word = OTG_FS_FIFO[0][0];
				pkt->data[i] = word;
				pkt->data[i + 1] = word >> 8;
				pkt->data[i + 2] = word >> 16;
				pkt->data[i + 3] = word >> 24;
			}

			// Push the packet onto the transmit queue
			if (last_out_pending) {
				last_out_pending->next = pkt;
				last_out_pending = pkt;
			} else {
				first_out_pending = last_out_pending = pkt;
			}

			// Kick the transmit queue
			push_mrf_tx();
		}
	} else if (pktsts == 0b0011) {
		// OUT transfer complete
		out_ep3_enabled = false;
		if (unreliable_out_free) {
			init_out_ep3();
		}
	}
}

static bool can_enter(void) {
	return config.pan_id != 0xFFFF;
}

static void on_enter(void) {
	// Initialize the linked lists
	unreliable_out_free = 0;
	for (size_t i = 0; i < sizeof(perconfig.normal.out_packets) / sizeof(*perconfig.normal.out_packets) / 2; ++i) {
		normal_out_packet_t *pkt = &perconfig.normal.out_packets[i];
		pkt->next = unreliable_out_free;
		unreliable_out_free = pkt;
	}
	reliable_out_free = 0;
	for (size_t i = sizeof(perconfig.normal.out_packets) / sizeof(*perconfig.normal.out_packets) / 2; i < sizeof(perconfig.normal.out_packets) / sizeof(*perconfig.normal.out_packets); ++i) {
		normal_out_packet_t *pkt = &perconfig.normal.out_packets[i];
		pkt->next = reliable_out_free;
		reliable_out_free = pkt;
	}
	in_free = 0;
	for (size_t i = 0; i < sizeof(perconfig.normal.in_packets) / sizeof(*perconfig.normal.in_packets); ++i) {
		normal_in_packet_t *pkt = &perconfig.normal.in_packets[i];
		pkt->next = in_free;
		in_free = pkt;
	}
	first_out_pending = last_out_pending = cur_out_transmitting = first_mdr_pending = last_mdr_pending = 0;

	// Initialize the radio
	poll_index = 0;
	mrf_tx_active = false;
	for (size_t i = 0; i < sizeof(mrf_rx_seqnum) / sizeof(*mrf_rx_seqnum); ++i) {
		mrf_rx_seqnum[i] = 0xFFFF;
	}
	mrf_init();
	sleep_100us(1);
	mrf_release_reset();
	sleep_100us(3);
	mrf_common_init();
	while (GPIOC_IDR & (1 << 12));
	mrf_write_short(MRF_REG_SHORT_SADRH, 0x01);
	mrf_write_short(MRF_REG_SHORT_SADRL, 0x00);
	mrf_analogue_txrx();
	mrf_write_short(MRF_REG_SHORT_INTCON, 0b11110110);
#warning set sleep clock stuff needed for beacon order for beaconed coordinator mode; not doing that yet
#if 0
	mrf_write_short(MRF_REG_SHORT_RXMCR, 0xC0); // Set as coordinator and PAN coordinator
#if 1
	mrf_write_short(MRF_REG_SHORT_TXMCR, (1 << 5) | (3 << 3) | (4 << 0)); // Enable slotted transmission mode
#endif
	// Load beacon
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 0, 11); // Header length
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 1, 16); // Frame length
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 2, 0b00000000); // Frame control LSB
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 3, 0b10000000); // Frame control MSB
#warning TODO pick the sequence number randomly
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 4, 0); // Sequence number
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 5, config.pan_id); // Source PAN ID LSB
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 6, config.pan_id >> 8); // Source PAN ID MSB
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 7, 0); // Source address LSB
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 8, 0); // Source address MSB
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 9, 0b11101110); // Superframe specification LSB
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 10, 0b11001110); // Superframe specification MSB
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 11, 0b00000000); // GTS specification
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 12, 0b00000000); // Pending address specification
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 13, 'T'); // Beacon payload
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 14, 'B'); // Beacon payload
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 15, 'o'); // Beacon payload
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 16, 't'); // Beacon payload
	mrf_write_long(MRF_REG_LONG_TXBFIFO + 17, 's'); // Beacon payload
	mrf_write_short(MRF_REG_SHORT_TXBCON1, 0xB0); // Mask out beacon transmit interrupt
	mrf_write_short(MRF_REG_SHORT_WAKECON, 0x83); // Set timing interval between triggering slotted mode and first beacon to 3
#if 1
	mrf_write_short(MRF_REG_SHORT_ESLOTG1, 0xEE); // No GTSs
	mrf_write_long(MRF_REG_LONG_SLPCAL2, 0x10); // Start sleep clock calibration
	while (!(mrf_read_long(MRF_REG_LONG_SLPCAL2) & 0x80)); // Wait until calibration complete
	mrf_write_short(MRF_REG_SHORT_SLPACK, 0x5F); // Set oscillator startup counter
	mrf_write_short(MRF_REG_SHORT_ORDER, 0xEE); // Set maximum beacon/superframe order
#if 0
	mrf_write_short(MRF_REG_SHORT_TXBCON0, 0x01); // Trigger beacon transmit
#endif
#endif
#endif

	// Turn on LED 1
	GPIOB_BSRR = 1 << 12;

	// Enable external interrupt on MRF INT rising edge
	interrupt_exti12_handler = &exti12_interrupt_vector;
	rcc_enable(APB2, 14);
	SYSCFG_EXTICR[12 / 4] = (SYSCFG_EXTICR[12 / 4] & ~(15 << (12 % 4))) | (2 << (12 % 4)); // EXTI12 = 2; map PC12 to EXTI12
	rcc_disable(APB2, 14);
	EXTI_RTSR |= 1 << 12; // TR12 = 1; enable rising edge trigger on EXTI12
	EXTI_FTSR &= ~(1 << 12); // TR12 = 0; disable falling edge trigger on EXTI12
	EXTI_IMR |= 1 << 12; // MR12 = 1; enable interrupt on EXTI12 trigger
	NVIC_ISER[40 / 32] = 1 << (40 % 32); // SETENA40 = 1; enable EXTI15…10 interrupt

	// Set up interrupt endpoint 1 OUT
	out_ep1_enabled = false;
	usb_out_set_callback(1, &on_ep1_out_pattern);
	OTG_FS_DOEPCTL1 =
		(1 << 28) // SD0PID + 1; set data 0 PID
		| (3 << 18) // EPTYP = 3; interrupt endpoint
		| (1 << 15) // USBAEP = 1; endpoint active in this configuration
		| (64 << 0); // MPSIZ = 64; max packet size is 64 bytes
	init_out_ep1();

	// Set up interrupt endpoint 2 OUT
	out_ep2_enabled = false;
	usb_out_set_callback(2, &on_ep2_out_pattern);
	OTG_FS_DOEPCTL2 =
		(1 << 28) // SD0PID + 1; set data 0 PID
		| (3 << 18) // EPTYP = 3; interrupt endpoint
		| (1 << 15) // USBAEP = 1; endpoint active in this configuration
		| (64 << 0); // MPSIZ = 64; max packet size is 64 bytes
	init_out_ep2();

	// Set up interrupt endpoint 3 OUT
	out_ep3_enabled = false;
	usb_out_set_callback(3, &on_ep3_out_pattern);
	OTG_FS_DOEPCTL3 =
		(1 << 28) // SD0PID = 1; set data 0 PID
		| (3 << 18) // EPTYP = 3; interrupt endpoint
		| (1 << 15) // USBAEP = 1; endpoint active in this configuration
		| (64 << 0); // MPSIZ = 64; max packet size is 64 bytes
	init_out_ep3();

	// Set up endpoint 1 IN
	in_ep1_enabled = false;
	usb_in_set_callback(1, &on_ep1_in_interrupt);
	OTG_FS_DIEPTXF1 =
		(16 << 16) // INEPTXFD = 16; allocate 16 words of FIFO space for this FIFO; this is larger than any transfer we will ever send, so we *never* need to deal with a full FIFO!
		| (usb_application_fifo_offset() << 0); // INEPTXSA = offset; place this FIFO after the endpoint-zero FIFOs
	OTG_FS_DIEPCTL1 =
		(0 << 31) // EPENA = 0; do not start transmission on this endpoint
		| (0 << 30) // EPDIS = 0; do not disable this endpoint at this time
		| (1 << 28) // SD0PID = 1; set data PID to 0
		| (1 << 27) // SNAK = 1; set NAK flag
		| (0 << 26) // CNAK = 0; do not clear NAK flag
		| (1 << 22) // TXFNUM = 1; use transmit FIFO number 1
		| (0 << 21) // STALL = 0; do not stall traffic
		| (3 << 18) // EPTYP = 3; interrupt endpoint
		| (1 << 15) // USBAEP = 1; endpoint is active in this configuration
		| (2 << 0); // MPSIZ = 2; maximum packet size is 2 bytes
	while (!(OTG_FS_DIEPCTL1 & (1 << 17) /* NAKSTS */));
	while (!(OTG_FS_GRSTCTL & (1 << 31) /* AHBIDL */));
	OTG_FS_GRSTCTL = (OTG_FS_GRSTCTL & 0x7FFFF808) // Reserved
		| (1 << 6) // TXFNUM = 1; flush transmit FIFO #1
		| (1 << 5); // TXFFLSH = 1; flush transmit FIFO
	while (OTG_FS_GRSTCTL & (1 << 5) /* TXFFLSH */);
	OTG_FS_DIEPINT1 = OTG_FS_DIEPINT1; // Clear all pending interrupts for IN endpoint 1
	OTG_FS_DAINTMSK |= 1 << 1; // IEPM1 = 1; enable interrupts for IN endpoint 1

	// Set up endpoint 2 IN
	in_ep2_enabled = false;
	in_ep2_zlp_pending = false;
	usb_in_set_callback(2, &on_ep2_in_interrupt);
	OTG_FS_DIEPTXF2 =
		(64 << 16) // INEPTXFD = 64; allocate 64 words of FIFO space for this FIFO; this is larger than any transfer we will ever send, so we *never* need to deal with a full FIFO!
		| ((usb_application_fifo_offset() + 16) << 0); // INEPTXSA = offset; place this FIFO after the endpoint-zero and endpoint-1 FIFOs
	OTG_FS_DIEPCTL2 =
		(0 << 31) // EPENA = 0; do not start transmission on this endpoint
		| (0 << 30) // EPDIS = 0; do not disable this endpoint at this time
		| (1 << 28) // SD0PID = 1; set data PID to 0
		| (1 << 27) // SNAK = 1; set NAK flag
		| (0 << 26) // CNAK = 0; do not clear NAK flag
		| (2 << 22) // TXFNUM = 2; use transmit FIFO number 2
		| (0 << 21) // STALL = 0; do not stall traffic
		| (3 << 18) // EPTYP = 3; interrupt endpoint
		| (1 << 15) // USBAEP = 1; endpoint is active in this configuration
		| (64 << 0); // MPSIZ = 64; maximum packet size is 64 bytes
	while (!(OTG_FS_DIEPCTL2 & (1 << 17) /* NAKSTS */));
	while (!(OTG_FS_GRSTCTL & (1 << 31) /* AHBIDL */));
	OTG_FS_GRSTCTL = (OTG_FS_GRSTCTL & 0x7FFFF808) // Reserved
		| (2 << 6) // TXFNUM = 2; flush transmit FIFO #2
		| (1 << 5); // TXFFLSH = 1; flush transmit FIFO
	while (OTG_FS_GRSTCTL & (1 << 5) /* TXFFLSH */);
	OTG_FS_DIEPINT2 = OTG_FS_DIEPINT2; // Clear all pending interrupts for IN endpoint 2
	OTG_FS_DAINTMSK |= 1 << 2; // IEPM2 = 1; enable interrupts for IN endpoint 2

	// Wipe the drive packet
	memset(perconfig.normal.drive_packet, 0, sizeof(perconfig.normal.drive_packet));
	drive_packet_pending = false;

	// Set up timer 6 to overflow every 20 milliseconds for the drive packet
	// Timer 6 input is 72 MHz from the APB
	// Need to count to 1,440,000 for each overflow
	// Set prescaler to 1,000, auto-reload to 1,440
	rcc_enable(APB1, 4);
	TIM6_CR1 = (TIM6_CR1 & 0b1111111101110000) // Reserved
		| (0 << 7) // ARPE = 0; ARR not buffered
		| (0 << 3) // OPM = 0; run continuously
		| (1 << 2) // URS = 1; only overflow generates an interrupt
		| (0 << 1) // UDIS = 0; updates not disabled
		| (0 << 0); // CEN = 0; counter not presently enabled
	TIM6_DIER = 1 << 0; // UIE = 1; update interrupt enabled
	TIM6_PSC = 999;
	TIM6_ARR = 1439;
	TIM6_CNT = 0;
	NVIC_ISER[54 / 32] = 1 << (54 % 32); // SETENA54 = 1; enable timer 6 interrupt
}

static void on_exit(void) {
	// Turn off timer 6
	TIM6_CR1 &= ~(1 << 0); // CEN = 0; disable counter
	NVIC_ICER[54 / 32] = 1 << (54 % 32); // CLRENA54 = 1; disable timer 6 interrupt
	rcc_disable(APB1, 4);

	// Shut down OUT endpoint 1
	if (OTG_FS_DOEPCTL1 & (1 << 31) /* EPENA */) {
		if (!(OTG_FS_DOEPCTL1 & (1 << 17) /* NAKSTS */)) {
			OTG_FS_DOEPCTL1 |= 1 << 27; // SNAK = 1; start NAKing traffic
			while (!(OTG_FS_DOEPCTL1 & (1 << 17) /* NAKSTS */));
		}
		OTG_FS_DOEPCTL1 |= 1 << 30; // EPDIS = 1; disable endpoint
		while (OTG_FS_DOEPCTL1 & (1 << 31) /* EPENA */);
	}
	usb_out_set_callback(1, 0);
	out_ep1_enabled = false;

	// Shut down OUT endpoint 2
	if (OTG_FS_DOEPCTL2 & (1 << 31) /* EPENA */) {
		if (!(OTG_FS_DOEPCTL2 & (1 << 17) /* NAKSTS */)) {
			OTG_FS_DOEPCTL2 |= 1 << 27; // SNAK = 1; start NAKing traffic
			while (!(OTG_FS_DOEPCTL2 & (1 << 17) /* NAKSTS */));
		}
		OTG_FS_DOEPCTL2 |= 1 << 30; // EPDIS = 1; disable endpoint
		while (OTG_FS_DOEPCTL2 & (1 << 31) /* EPENA */);
	}
	usb_out_set_callback(2, 0);
	out_ep2_enabled = false;

	// Shut down OUT endpoint 3
	if (OTG_FS_DOEPCTL3 & (1 << 31) /* EPENA */) {
		if (!(OTG_FS_DOEPCTL3 & (1 << 17) /* NAKSTS */)) {
			OTG_FS_DOEPCTL3 |= 1 << 27; // SNAK = 1; start NAKing traffic
			while (!(OTG_FS_DOEPCTL3 & (1 << 17) /* NAKSTS */));
		}
		OTG_FS_DOEPCTL3 |= 1 << 30; // EPDIS = 1; disable endpoint
		while (OTG_FS_DOEPCTL3 & (1 << 31) /* EPENA */);
	}
	usb_out_set_callback(3, 0);
	out_ep3_enabled = false;

	// Shut down IN endpoint 1
	if (OTG_FS_DIEPCTL1 & (1 << 31) /* EPENA */) {
		if (!(OTG_FS_DIEPCTL1 & (1 << 17) /* NAKSTS */)) {
			OTG_FS_DIEPCTL1 |= 1 << 27; // SNAK = 1; start NAKing traffic
			while (!(OTG_FS_DIEPCTL1 & (1 << 17) /* NAKSTS */));
		}
		OTG_FS_DIEPCTL1 |= 1 << 30; // EPDIS = 1; disable endpoint
		while (OTG_FS_DIEPCTL1 & (1 << 31) /* EPENA */);
	}
	OTG_FS_DIEPCTL1 = 0;
	OTG_FS_DAINTMSK &= ~(1 << 1); // IEPM1 = 0; disable general interrupts for IN endpoint 1
	OTG_FS_DIEPEMPMSK &= ~(1 << 1); // INEPTXFEM1 = 0; disable FIFO empty interrupts for IN endpoint 1
	usb_in_set_callback(1, 0);
	in_ep1_enabled = false;

	// Shut down IN endpoint 2
	if (OTG_FS_DIEPCTL2 & (1 << 31) /* EPENA */) {
		if (!(OTG_FS_DIEPCTL2 & (1 << 17) /* NAKSTS */)) {
			OTG_FS_DIEPCTL2 |= 1 << 27; // SNAK = 1; start NAKing traffic
			while (!(OTG_FS_DIEPCTL2 & (1 << 17) /* NAKSTS */));
		}
		OTG_FS_DIEPCTL2 |= 1 << 30; // EPDIS = 1; disable endpoint
		while (OTG_FS_DIEPCTL2 & (1 << 31) /* EPENA */);
	}
	OTG_FS_DIEPCTL2 = 0;
	OTG_FS_DAINTMSK &= ~(1 << 2); // IEPM2 = 0; disable general interrupts for IN endpoint 2
	OTG_FS_DIEPEMPMSK &= ~(1 << 2); // INEPTXFEM2 = 0; disable FIFO empty interrupts for IN endpoint 2
	usb_in_set_callback(2, 0);
	in_ep2_enabled = false;

	// Disable the external interrupt on MRF INT
	NVIC_ICER[40 / 32] = 1 << (40 % 32); // CLRENA40 = 1; disable EXTI15…10 interrupt
	EXTI_IMR &= ~(1 << 12); // MR12 = 0; disable interrupt on EXTI12 trigger
	interrupt_exti12_handler = 0;

	// Turn off all LEDs
	GPIOB_BSRR = 7 << (12 + 16);

	// Reset the radio
	mrf_init();
}

static bool on_in_request(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, uint16_t length __attribute__((unused)), usb_ep0_source_t **source) {
	static usb_ep0_memory_source_t mem_src;

	if (request_type == 0xC0 && request == 0x00 && !value && !index) {
		// GET CHANNEL
		*source = usb_ep0_memory_source_init(&mem_src, &config.channel, sizeof(config.channel));
		return true;
	} else if (request_type == 0xC0 && request == 0x02 && !value && !index) {
		// GET SYMBOL RATE
		*source = usb_ep0_memory_source_init(&mem_src, &config.symbol_rate, sizeof(config.symbol_rate));
		return true;
	} else if (request_type == 0xC0 && request == 0x04 && !value && !index) {
		// GET PAN ID
		*source = usb_ep0_memory_source_init(&mem_src, &config.pan_id, sizeof(config.pan_id));
		return true;
	} else if (request_type == 0xC0 && request == 0x06 && !value && !index) {
		// GET MAC ADDRESS
		*source = usb_ep0_memory_source_init(&mem_src, &config.mac_address, sizeof(config.mac_address));
		return true;
	} else {
		return false;
	}
}

const usb_ep0_configuration_callbacks_t CONFIGURATION_CBS2 = {
	.configuration = 2,
	.interfaces = 1,
	.out_endpoints = 3,
	.in_endpoints = 3,
	.can_enter = &can_enter,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
	.on_zero_request = 0,
	.on_in_request = &on_in_request,
	.on_out_request = 0,
};

