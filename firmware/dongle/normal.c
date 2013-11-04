#include "normal.h"
#include "config.h"
#include "constants.h"
#include "estop.h"
#include "mrf.h"
#include "perconfig.h"
#include <exti.h>
#include <gpio.h>
#include <rcc.h>
#include <registers/exti.h>
#include <registers/nvic.h>
#include <registers/timer.h>
#include <sleep.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unused.h>
#include <usb_bi_in.h>
#include <usb_bi_out.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>
#include <usb_fifo.h>

#define PKT_FLAG_RELIABLE 0x01

static bool drive_packet_pending, drive_packet_halt_pending, mrf_tx_active;
static estop_t last_reported_estop_value;
static uint8_t mrf_tx_seqnum;
static uint16_t mrf_rx_seqnum[8];
static normal_out_packet_t *unreliable_out_free, *reliable_out_free, *first_out_pending, *last_out_pending, *cur_out_transmitting, *first_mdr_pending, *last_mdr_pending;
static normal_in_packet_t *in_free, *first_in_pending, *last_in_pending;
static unsigned int poll_index;

static void send_drive_packet(void) {
	// Write out the packet.
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
	if (estop_read() != ESTOP_RUN) {
		for (size_t i = 0; i < sizeof(perconfig.normal.drive_packet) - 1; i += 8) {
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i + 1, (i == poll_index * (sizeof(perconfig.normal.drive_packet) - 1) / 8) ? 0b10000000 : 0b00000000);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i + 0, 0x00);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i + 3, 0b01000000);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i + 2, 0x00);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i + 5, 0x00);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i + 4, 0x00);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i + 7, 0x00);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i + 6, 0x00);
		}
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + sizeof(perconfig.normal.drive_packet) - 1, perconfig.normal.drive_packet[sizeof(perconfig.normal.drive_packet) - 1]);
	} else {
		for (size_t i = 0; i < sizeof(perconfig.normal.drive_packet); ++i) {
			uint8_t mask = 0;
			if (i - 1 == poll_index * sizeof(perconfig.normal.drive_packet) / 8) {
				mask = 0x80;
			}
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i, bptr[i] | mask);
		}
	}
	poll_index = (poll_index + 1) % 8;

	// Initiate transmission with no acknowledgement.
	mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000001);
	mrf_tx_active = true;
	drive_packet_pending = false;

	// Blink the transmit light.
	gpio_toggle(GPIOB, 13);
}

void timer6_interrupt_vector(void) {
	// Clear interrupt flag.
	{
		TIM_basic_SR_t tmp = { 0 };
		TIM6.SR = tmp;
	}

	// Check if a packet is currently being transmitted.
	if (!mrf_tx_active) {
		send_drive_packet();
	} else {
		drive_packet_pending = true;
	}
}

static void push_mrf_tx(void) {
	// Don’t try to transmit if already doing so.
	if (mrf_tx_active) {
		return;
	}

	// Decide what type of packet we should send.
	if (drive_packet_pending) {
		// A timer interrupt indicated we should send a drive packet, but the transmitter was active; send it now.
		send_drive_packet();
	} else if (first_out_pending) {
		// No need to send a drive packet right now, but there is an ordinary data packet waiting; send that.
		// Pop the packet from the queue, if there is no current packet.
		cur_out_transmitting = first_out_pending;
		first_out_pending = cur_out_transmitting->next;
		if (!first_out_pending) {
			last_out_pending = 0;
		}
		cur_out_transmitting->next = 0;
		++mrf_tx_seqnum;

		// Write the packet into the transmit buffer.
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 0, 9); // Header length
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 1, 9 + cur_out_transmitting->length); // Frame length
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 2, 0b01100001); // Frame control LSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 3, 0b10001000); // Frame control MSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 4, mrf_tx_seqnum); // Sequence number
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 5, config.pan_id); // Destination PAN ID LSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 6, config.pan_id >> 8); // Destination PAN ID MSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 7, cur_out_transmitting->dest); // Destination address LSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 8, 0x00); // Destination address MSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 9, 0x00); // Source address LSB
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 10, 0x01); // Source address MSB
		for (size_t i = 0; i < cur_out_transmitting->length; ++i) {
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11 + i, cur_out_transmitting->data[i]);
		}

		// Initiate transmission with acknowledgement.
		mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);
		mrf_tx_active = true;
	}
}

static void push_mdrs(void) {
	// If the message delivery endpoint is already running a transfer, or it is halted, we have nothing to do.
	// We will get back here later when the transfer complete notification occurs for this endpoint and we can then push more MDRs.
	usb_bi_in_state_t state = usb_bi_in_get_state(1);
	if (state == USB_BI_IN_STATE_ACTIVE || state == USB_BI_IN_STATE_HALTED) {
		return;
	}

	// If there are no MDRs waiting, do nothing.
	// We will get back here later when an MRF interrupt leads to an MDR being produced and we can then push the MDR.
	if (!first_mdr_pending) {
		return;
	}

	// Pop up to four MDRs from the queue.
	normal_out_packet_t *pkts = 0;
	unsigned int num_pkts = 0;
	while (first_mdr_pending && num_pkts < 4) {
		normal_out_packet_t *pkt = first_mdr_pending;
		first_mdr_pending = pkt->next;
		if (!first_mdr_pending) {
			last_mdr_pending = 0;
		}
		pkt->next = pkts;
		pkts = pkt;
		++num_pkts;
	}

	// Start a transfer.
	usb_bi_in_start_transfer(1, 2 * num_pkts, 8, &push_mdrs, 0);

	while (pkts) {
		// Pop a packet from the queue of MDRs to use in this transfer.
		normal_out_packet_t *pkt = pkts;
		pkts = pkt->next;
		pkt->next = 0;

		// Push the data for this MDR.
		usb_bi_in_push(1, &pkt->message_id, 1);
		usb_bi_in_push(1, &pkt->delivery_status, 1);

		// Push the consumed MDR packet buffer onto the free stack
		pkt->next = reliable_out_free;
		reliable_out_free = pkt;
	}
}

static void push_rx(void) {
	// If the received message endpoint already has data queued, or it is halted, don’t try to send more data.
	// We will get back here later when the transfer complete notification occurs for this endpoint and we can push more messages.
	usb_bi_in_state_t state = usb_bi_in_get_state(2);
	if (state == USB_BI_IN_STATE_ACTIVE || state == USB_BI_IN_STATE_HALTED) {
		return;
	}

	// If there are no received messages waiting to be sent, do nothing.
	// We will get back here later when an MRF interrupt leads to a received message being queued and we can then push the message.
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

	// Start a transfer.
	// Received message transfers are variable length up to a known maximum size.
	usb_bi_in_start_transfer(2, packet->length, sizeof(packet->data), &push_rx, 0);

	// Push the data for this transfer.
	usb_bi_in_push(2, packet->data, packet->length);

	// Push this consumed packet buffer into the free list.
	packet->next = in_free;
	in_free = packet;

	// Re-enable the receiver; it may have been left disabled if there were no free receive message buffers.
	mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
}

static void push_estop(void) {
	// If the estop endpoint already has data queued, or it is halted, don’t try to send more data.
	// We will get back here later when the transfer complete notification occurs for this endpoint and we can push more data.
	usb_bi_in_state_t state = usb_bi_in_get_state(3);
	if (state == USB_BI_IN_STATE_ACTIVE || state == USB_BI_IN_STATE_HALTED) {
		return;
	}

	// Read the current estop state.
	estop_t current_value = estop_read();

	// If there is nothing interesting to say, don’t try to send more data.
	// We will get back here later when the estop status changes and we can push more data.
	if (current_value == last_reported_estop_value) {
		return;
	}

	// Start a transfer.
	// Estop transfers are always two bytes long.
	usb_bi_in_start_transfer(3, 1, 1, &push_estop, 0);

	// Push the data for this transfer.
	usb_bi_in_push(3, &current_value, 1);

	// Record the current value as the last reported.
	last_reported_estop_value = current_value;
}

static void exti12_interrupt_vector(void) {
	// Clear the interrupt.
	EXTI_PR = 1 << 12; // PR12 = 1; clear pending EXTI12 interrupt

	while (mrf_get_interrupt()) {
		// Check outstanding interrupts.
		uint8_t intstat = mrf_read_short(MRF_REG_SHORT_INTSTAT);
		if (intstat & (1 << 3)) {
			// RXIF = 1; packet received.
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception
			// Read out the frame length byte and frame control word.
			uint8_t rxfifo_frame_length = mrf_read_long(MRF_REG_LONG_RXFIFO);
			uint16_t frame_control = mrf_read_long(MRF_REG_LONG_RXFIFO + 1) | (mrf_read_long(MRF_REG_LONG_RXFIFO + 2) << 8);
			// Sanity-check the frame control word.
			if (((frame_control >> 0) & 7) == 1 /* Data packet */ && ((frame_control >> 3) & 1) == 0 /* No security */ && ((frame_control >> 6) & 1) == 1 /* Intra-PAN */ && ((frame_control >> 10) & 3) == 2 /* 16-bit destination address */ && ((frame_control >> 14) & 3) == 2 /* 16-bit source address */) {
				static const uint8_t HEADER_LENGTH = 2 /* Frame control */ + 1 /* Seq# */ + 2 /* Dest PAN */ + 2 /* Dest */ + 2 /* Src */;
				static const uint8_t FOOTER_LENGTH = 2 /* Frame check sequence */;

				// Sanity-check the total frame length.
				if (HEADER_LENGTH + FOOTER_LENGTH <= rxfifo_frame_length && rxfifo_frame_length <= HEADER_LENGTH + 102 + FOOTER_LENGTH) {
					// Read out and check the source address and sequence number.
					uint16_t source_address = mrf_read_long(MRF_REG_LONG_RXFIFO + 8) | (mrf_read_long(MRF_REG_LONG_RXFIFO + 9) << 8);
					uint8_t sequence_number = mrf_read_long(MRF_REG_LONG_RXFIFO + 3);
					if (source_address < 8 && sequence_number != mrf_rx_seqnum[source_address]) {
						// Blink the receive light.
						gpio_toggle(GPIOB, 14);

						// Update sequence number.
						mrf_rx_seqnum[source_address] = sequence_number;

						// Allocate a packet buffer.
						normal_in_packet_t *packet = in_free;
						in_free = packet->next;
						packet->next = 0;

						// Fill in the packet buffer.
						packet->length = 1 /* Source robot index */ + (rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH) /* 802.15.4 packet payload */ + 2;
						packet->data[0] = source_address;
						for (uint8_t i = 0; i < rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH; ++i) {
							packet->data[i + 1] = mrf_read_long(MRF_REG_LONG_RXFIFO + 1 + HEADER_LENGTH + i);
						}
						packet->data[1 + rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH] = mrf_read_long(MRF_REG_LONG_RXFIFO + 1 + rxfifo_frame_length); // LQI
						packet->data[1 + rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH + 1] = mrf_read_long(MRF_REG_LONG_RXFIFO + 1 + rxfifo_frame_length + 1); // RSSI

						// Push the packet on the receive queue.
						if (last_in_pending) {
							last_in_pending->next = packet;
							last_in_pending = packet;
						} else {
							first_in_pending = last_in_pending = packet;
						}

						// Kick the receive queue.
						push_rx();
					}
				}
			}

			// Only re-enable the receiver if we have a free packet buffer into which to receive additional data.
			// Not re-enabling the receiver if we don’t have a packet buffer will cause the MRF not to ACK,
			// which serves as flow control and guarantees that every ACKed packet is, in fact, delivered.
			if (in_free) {
				mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
			}
		}
		if (intstat & (1 << 0)) {
			// TXIF = 1; transmission complete (successful or failed).
			if (cur_out_transmitting) {
				// This is a unicast packet.
				// Read the transmission status to determine the packet’s fate.
				normal_out_packet_t *pkt = cur_out_transmitting;
				cur_out_transmitting = 0;
				uint8_t txstat = mrf_read_short(MRF_REG_SHORT_TXSTAT);
				if (txstat & 0x01) {
					// Transmission failed.
					if (txstat & (1 << 5)) {
						// CCA failed.
						pkt->delivery_status = MDR_STATUS_NO_CLEAR_CHANNEL;
					} else {
						// Assume: No ACK.
#warning possibly bad assumption; add a code for "unknown reason"
						pkt->delivery_status = MDR_STATUS_NOT_ACKNOWLEDGED;
					}
				} else {
					// Transmission successful.
					pkt->delivery_status = MDR_STATUS_OK;
				}

				// Decide whether to try transmission again or to retire the packet.
				if (pkt->delivery_status == MDR_STATUS_OK || !pkt->retries_remaining) {
					// Push the packet buffer onto the message delivery report queue (if reliable, else free).
					if (pkt->reliable) {
						// Save the message delivery report.
						if (last_mdr_pending) {
							last_mdr_pending->next = pkt;
							last_mdr_pending = pkt;
						} else {
							first_mdr_pending = last_mdr_pending = pkt;
						}

						// Kick the MDR queue.
						push_mdrs();
					} else {
						pkt->next = unreliable_out_free;
						unreliable_out_free = pkt;
					}
				} else {
					// We need to try the packet again later.
					// Push it to the front of the transmit queue.
					// This way, if a unicast packet fails, we will have a chance to send a drive packet before trying the unicast packet again.
					// This eliminates the possibility of a failed unicast packet tying up the radio for a long time and preventing drive packets from flowing.
					// We would like to push it to the BACK of the queue to avoid delaying packets to other robots.
					// However, we cannot do that because it might get out-of-order with other packets for the same bot.
					// This will be future work, to have separate per-robot transmit queues for better QoS/performance isolation.
					--pkt->retries_remaining;
					pkt->next = first_out_pending;
					first_out_pending = pkt;
					if (!last_out_pending) {
						last_out_pending = pkt;
					}
				}
			}

			// The transmitter is no longer active.
			mrf_tx_active = false;

			// Kick the transmit queue.
			push_mrf_tx();
		}
	}
}

static void on_ep1_out_packet(size_t bcnt) {
	if (bcnt == sizeof(perconfig.normal.drive_packet) - 1) {
		// Copy the received data into the temporary packet buffer.
		uint16_t temp[8][4];
		usb_bi_out_read(1, temp, sizeof(temp));
		// Check for any packet with the MSb of the first word set, which is illegal.
		bool legal = true;
		for (unsigned int i = 0; i < 8; ++i) {
			if (temp[i][0] & 0x8000) {
				legal = false;
			}
		}
		// Handle the packet.
		if (legal) {
			// Copy to final buffer.
			memcpy(perconfig.normal.drive_packet, temp, sizeof(temp));

			// Increment drive data counter.
			++perconfig.normal.drive_packet[sizeof(perconfig.normal.drive_packet) - 1];
		} else {
			// Halt the endpoint later.
			drive_packet_halt_pending = true;
		}
		// Enable the timer that generates drive packets on the radio.
		TIM6.CR1.CEN = 1; // Enable counter now
	}
}

static void start_ep1_out_transfer(void) {
	// Start another transfer, if not halted and not halt pending.
	if (drive_packet_halt_pending) {
		usb_bi_out_halt(1);
		drive_packet_halt_pending = false;
	} else if (usb_bi_out_get_state(1) != USB_BI_OUT_STATE_HALTED) {
		usb_bi_out_start_transfer(1, sizeof(perconfig.normal.drive_packet) - 1, &start_ep1_out_transfer, &on_ep1_out_packet);
	}
}

static void handle_ep1o_clear_halt(unsigned int UNUSED(ep)) {
	start_ep1_out_transfer();
}

static void handle_ep1i_clear_halt(unsigned int UNUSED(ep)) {
	// Free all the queued MDRs.
	while (first_mdr_pending) {
		normal_out_packet_t *pkt = first_mdr_pending;
		first_mdr_pending = pkt->next;
		pkt->next = reliable_out_free;
		reliable_out_free = pkt;
	}
	last_mdr_pending = 0;
}

static void on_ep2_out_packet(size_t bcnt) {
	if (bcnt >= 2) {
		// Allocate a packet buffer.
		normal_out_packet_t *pkt = reliable_out_free;
		reliable_out_free = pkt->next;
		pkt->next = 0;

		// Flag the packet as reliable.
		pkt->reliable = true;

		// The transfer has a two-byte header.
		pkt->length = bcnt - 2;

		// Allow up to twenty transport-layer retries.
		pkt->retries_remaining = 20;

		// Copy out the header and data.
		uint8_t destination_and_priority;
		usb_bi_out_read(2, &destination_and_priority, 1);
		pkt->dest = destination_and_priority & 0x0F;
		usb_bi_out_read(2, &pkt->message_id, 1);
		usb_bi_out_read(2, pkt->data, bcnt - 2);

		// Push the packet onto the transmit queue.
		if (last_out_pending) {
			last_out_pending->next = pkt;
			last_out_pending = pkt;
		} else {
			first_out_pending = last_out_pending = pkt;
		}

		// Kick the transmit queue.
		push_mrf_tx();
	}
}

static void start_ep2_out_transfer(void);

static void on_ep2_out_transfer_complete(void) {
	if (reliable_out_free) {
		start_ep2_out_transfer();
	}
}

static void start_ep2_out_transfer(void) {
#warning this endpoint really ought to handle transfers of more than 64 bytes
	// Start another transfer.
	usb_bi_out_start_transfer(2, 64, &on_ep2_out_transfer_complete, &on_ep2_out_packet);
}

static void handle_ep2o_clear_halt(unsigned int UNUSED(ep)) {
	start_ep2_out_transfer();
}

static void handle_ep2i_clear_halt(unsigned int UNUSED(ep)) {
	// Free all the queued packets.
	while (first_in_pending) {
		normal_in_packet_t *pkt = first_in_pending;
		first_in_pending = pkt->next;
		pkt->next = in_free;
		in_free = pkt;
	}
	last_in_pending = 0;
}

static void on_ep3_out_packet(size_t bcnt) {
	if (bcnt) {
		// Allocate a packet buffer.
		normal_out_packet_t *pkt = unreliable_out_free;
		unreliable_out_free = pkt->next;
		pkt->next = 0;

		// Flag the packet as unreliable.
		pkt->reliable = false;

		// The transfer has a one-byte header.
		pkt->length = bcnt - 1;

		// Allow up to twenty transport-layer retries.
		pkt->retries_remaining = 20;

		// Copy out the header and data.
		uint8_t destination_and_priority;
		usb_bi_out_read(3, &destination_and_priority, 1);
		pkt->dest = destination_and_priority & 0x0F;
		usb_bi_out_read(3, pkt->data, bcnt - 1);

		// Push the packet onto the transmit queue.
		if (last_out_pending) {
			last_out_pending->next = pkt;
			last_out_pending = pkt;
		} else {
			first_out_pending = last_out_pending = pkt;
		}

		// Kick the transmit queue.
		push_mrf_tx();
	}
}

static void start_ep3_out_transfer(void);

static void on_ep3_out_transfer_complete(void) {
	if (unreliable_out_free) {
		start_ep3_out_transfer();
	}
}

static void start_ep3_out_transfer(void) {
#warning this endpoint really ought to handle transfers of more than 64 bytes
	usb_bi_out_start_transfer(3, 64, &on_ep3_out_transfer_complete, &on_ep3_out_packet);
}

static void handle_ep3o_clear_halt(unsigned int UNUSED(ep)) {
	start_ep3_out_transfer();
}

static void handle_ep3i_clear_halt(unsigned int UNUSED(ep)) {
	// Report the current state.
	push_estop();
}

static bool can_enter(void) {
	return config.pan_id != 0xFFFF;
}

static void on_enter(void) {
	// Initialize the linked lists.
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

	// Initialize the radio.
	poll_index = 0;
	mrf_tx_active = false;
	for (size_t i = 0; i < sizeof(mrf_rx_seqnum) / sizeof(*mrf_rx_seqnum); ++i) {
		mrf_rx_seqnum[i] = 0xFFFF;
	}
	mrf_init();
	sleep_us(100);
	mrf_release_reset();
	sleep_us(300);
	mrf_common_init();
	while (mrf_get_interrupt());
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
	// Load beacon.
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

	// Turn on LED 1.
	gpio_set(GPIOB, 12);

	// Enable external interrupt on MRF INT rising edge.
	exti_set_handler(12, &exti12_interrupt_vector);
	exti_map(12, 2); // Map PC12 to EXTI12
	EXTI_RTSR |= 1 << 12; // TR12 = 1; enable rising edge trigger on EXTI12
	EXTI_FTSR &= ~(1 << 12); // TR12 = 0; disable falling edge trigger on EXTI12
	EXTI_IMR |= 1 << 12; // MR12 = 1; enable interrupt on EXTI12 trigger
	NVIC_ISER[40 / 32] = 1 << (40 % 32); // SETENA40 = 1; enable EXTI15…10 interrupt

	// Set up interrupt endpoint 1 OUT.
	usb_bi_out_init(1, 64, USB_BI_OUT_EP_TYPE_INTERRUPT);
	usb_bi_out_set_std_halt(1, 0, 0, &handle_ep1o_clear_halt);
	start_ep1_out_transfer();

	// Set up interrupt endpoint 2 OUT.
	usb_bi_out_init(2, 64, USB_BI_OUT_EP_TYPE_INTERRUPT);
	usb_bi_out_set_std_halt(2, 0, 0, &handle_ep2o_clear_halt);
	start_ep2_out_transfer();

	// Set up interrupt endpoint 3 OUT.
	usb_bi_out_init(3, 64, USB_BI_OUT_EP_TYPE_INTERRUPT);
	usb_bi_out_set_std_halt(3, 0, 0, &handle_ep3o_clear_halt);
	start_ep3_out_transfer();

	// Set up endpoint 1 IN with a 64-byte FIFO, large enough for any transfer (thus we never need to use the on_space callback).
	usb_fifo_enable(1, 64);
	usb_fifo_flush(1);
	usb_bi_in_init(1, 8, USB_BI_IN_EP_TYPE_BULK);
	usb_bi_in_set_std_halt(1, 0, 0, &handle_ep1i_clear_halt);

	// Set up endpoint 2 IN with a 256-byte FIFO, large enough for any transfer (thus we never need to use the on_space callback).
	usb_fifo_enable(2, 256);
	usb_fifo_flush(2);
	usb_bi_in_init(2, 64, USB_BI_IN_EP_TYPE_INTERRUPT);
	usb_bi_in_set_std_halt(2, 0, 0, &handle_ep2i_clear_halt);

	// Set up endpoint 3 IN with a 64-byte FIFO, large enough for any transfer (thus we never need to use the on_space callback).
	usb_fifo_enable(3, 64);
	usb_fifo_flush(3);
	usb_bi_in_init(3, 1, USB_BI_IN_EP_TYPE_INTERRUPT);
	usb_bi_in_set_std_halt(3, 0, 0, &handle_ep3i_clear_halt);

	// Wipe the drive packet.
	memset(perconfig.normal.drive_packet, 0, sizeof(perconfig.normal.drive_packet));
	drive_packet_pending = false;
	drive_packet_halt_pending = false;

	// Set up to be notified on estop changes and push the current state.
	last_reported_estop_value = ESTOP_STOP;
	estop_set_change_callback(&push_estop);
	push_estop();

	// Set up timer 6 to overflow every 20 milliseconds for the drive packet.
	// Timer 6 input is 72 MHz from the APB.
	// Need to count to 1,440,000 for each overflow.
	// Set prescaler to 1,000, auto-reload to 1,440.
	rcc_enable(APB1, TIM6);
	rcc_reset(APB1, TIM6);
	{
		TIM_basic_CR1_t tmp = {
			.ARPE = 0, // ARR is not buffered.
			.OPM = 0, // Counter counters forever.
			.URS = 1, // Update interrupts and DMA requests generated only at counter overflow.
			.UDIS = 0, // Updates not inhibited.
			.CEN = 0, // Timer not currently enabled.
		};
		TIM6.CR1 = tmp;
	}
	{
		TIM_basic_DIER_t tmp = {
			.UDE = 0, // DMA disabled.
			.UIE = 1, // Interrupt enabled.
		};
		TIM6.DIER = tmp;
	}
	TIM6.PSC = 999;
	TIM6.ARR = 1439;
	TIM6.CNT = 0;
	NVIC_ISER[54 / 32] = 1 << (54 % 32); // SETENA54 = 1; enable timer 6 interrupt
}

static void on_exit(void) {
	// Turn off timer 6.
	{
		TIM_basic_CR1_t tmp = { 0 };
		TIM6.CR1 = tmp; // Disable counter
	}
	NVIC_ICER[54 / 32] = 1 << (54 % 32); // CLRENA54 = 1; disable timer 6 interrupt
	rcc_disable(APB1, TIM6);

	// Stop receiving estop notifications.
	estop_set_change_callback(0);

	// Shut down endpoints.
	usb_bi_out_deinit(1);
	usb_bi_out_deinit(2);
	usb_bi_out_deinit(3);
	usb_bi_in_deinit(1);
	usb_bi_in_deinit(2);
	usb_bi_in_deinit(3);

	// Deallocate FIFOs.
	usb_fifo_disable(3);
	usb_fifo_disable(2);
	usb_fifo_disable(1);

	// Disable the external interrupt on MRF INT.
	NVIC_ICER[40 / 32] = 1 << (40 % 32); // CLRENA40 = 1; disable EXTI15…10 interrupt
	EXTI_IMR &= ~(1 << 12); // MR12 = 0; disable interrupt on EXTI12 trigger
	exti_set_handler(12, 0);

	// Turn off all LEDs.
	gpio_set_reset_mask(GPIOB, 0, 7 << 12);

	// Reset the radio.
	mrf_init();
}

const usb_altsettings_altsetting_t NORMAL_ALTSETTING = {
	.can_enter = &can_enter,
	.on_enter = &on_enter,
	.on_exit = &on_exit,
};

