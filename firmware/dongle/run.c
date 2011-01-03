#include "run.h"
#include "bulk_out.h"
#include "critsec.h"
#include "debug.h"
#include "dongle_status.h"
#include "global.h"
#include "interrupt_out.h"
#include "local_error_queue.h"
#include "pipes.h"
#include "queue.h"
#include "state_transport_in.h"
#include "state_transport_out.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define POLL_TIMEOUT 20

#define XBEE_API_ID_TX16 0x01
#define XBEE_API_ID_RX16 0x81
#define XBEE_API_ID_TX_STATUS 0x89

#define XBEE_TX16_OPTION_DISABLE_ACK 0x01
#define XBEE_TX16_OPTION_BROADCAST_PAN 0x04

typedef struct {
	uint8_t api_id;
	uint8_t frame_id;
	uint8_t address_high;
	uint8_t address_low;
	uint8_t options;
} xbee_tx16_header_t;

typedef struct {
	unsigned sequence : 6;
	unsigned used : 1;
} pipe_info_t;

typedef struct {
	unsigned pipe : 4;
	unsigned robot : 4;
} drive_micropacket_pipe_robot_t;

typedef struct {
	uint8_t micropacket_length;
	drive_micropacket_pipe_robot_t pipe_robot;
	uint8_t payload[STATE_TRANSPORT_OUT_DRIVE_SIZE];
} drive_micropacket_t;

typedef struct {
	xbee_tx16_header_t xbee_header;
	uint8_t pipe;
	uint8_t sequence;
} interrupt_header_t;

typedef struct {
	unsigned sequence : 6;
	unsigned last_micropacket : 1;
	unsigned first_micropacket : 1;
} bulk_sequence_flags_t;

typedef struct {
	xbee_tx16_header_t xbee_header;
	uint8_t pipe;
	bulk_sequence_flags_t sequence_flags;
} bulk_header_t;

typedef struct {
	xbee_tx16_header_t xbee_header;
	uint8_t flag;
} discovery_header_t;

void run(void) {
	static const uint8_t ZERO = 0;
	static const xbee_tx16_header_t DRIVE_XBEE_HEADER = { XBEE_API_ID_TX16, 0x00, 0xFF, 0xFF, XBEE_TX16_OPTION_DISABLE_ACK };
	uint8_t num_drive_sent;
	static drive_micropacket_t drive_micropackets[15];
	static xbee_txpacket_iovec_t drive_iovecs[2][3];
	static xbee_txpacket_t drive_packets[2];
	uint8_t pollee = 0;
	uint16_t last_poll_time = (UFRMH << 8) | UFRML;
	BOOL in_pending = false;

	uint8_t num_interrupt_out_sent;
	__data interrupt_out_packet_t *interrupt_out_packet;
	static __data interrupt_out_packet_t *interrupt_out_sent[3];
	static interrupt_header_t interrupt_out_headers[3];
	static xbee_txpacket_iovec_t interrupt_out_iovecs[3][2];
	static xbee_txpacket_t interrupt_out_packets[3];

	BOOL bulk_skip_message = false;
	uint8_t num_bulk_out_sent;
	__data bulk_out_packet_t *bulk_out_packet;
	uint8_t bulk_out_current_recipient = 0;
	static __data bulk_out_packet_t *bulk_out_sent_usb_packets[6];
	static bulk_header_t bulk_out_headers[3];
	static xbee_txpacket_iovec_t bulk_out_iovecs[3][4];
	static xbee_txpacket_t bulk_out_packets[3];
	uint8_t bulk_out_micropacket_lengths[3];

	uint8_t discovery_counter = 5, discovery_robot = 0;
	static discovery_header_t discovery_header;
	static xbee_txpacket_iovec_t discovery_iovec;
	static xbee_txpacket_t discovery_packet;
	BOOL discovery_sent;

	uint16_t now, mask;
	uint8_t next_frame = 1;
	static uint8_t comm_failures[15];
	static pipe_info_t pipe_info[15][PIPE_MAX + 1];
	CRITSEC_DECLARE(cs);

	/* Clear state. */
	{
		uint8_t i;
		for (i = 0; i != 15; ++i) {
			drive_micropackets[i].micropacket_length = sizeof(*drive_micropackets);
		}
		for (i = 0; i != 3; ++i) {
			interrupt_out_headers[i].xbee_header.api_id = XBEE_API_ID_TX16;
			interrupt_out_headers[i].xbee_header.address_high = 0x7B;
			interrupt_out_headers[i].xbee_header.options = 0;
		}
		for (i = 0; i != 3; ++i) {
			bulk_out_headers[i].xbee_header.api_id = XBEE_API_ID_TX16;
			bulk_out_headers[i].xbee_header.address_high = 0x7B;
			bulk_out_headers[i].xbee_header.options = 0;
		}
	}
	discovery_header.xbee_header.api_id = XBEE_API_ID_TX16;
	discovery_header.xbee_header.address_high = 0x7B;
	discovery_header.xbee_header.options = 0;
	discovery_header.flag = 0xFF;
	memset(comm_failures, 0, sizeof(comm_failures));
	{
		uint8_t i, j;
		for (i = 0; i != 15; ++i) {
			for (j = 0; j != PIPE_MAX + 1; ++j) {
				if (pipe_out_mask & (1 << j)) {
					pipe_info[i][j].used = 0;
					pipe_info[i][j].sequence = 0;
				} else {
					pipe_info[i][j].used = 0;
					pipe_info[i][j].sequence = 63;
				}
			}
		}
	}

	while (!should_shut_down) {
		/* Initialize per-cycle state. */
		num_drive_sent = 0;
		num_interrupt_out_sent = 0;
		num_bulk_out_sent = 0;
		memset(bulk_out_sent_usb_packets, 0, sizeof(bulk_out_sent_usb_packets));
		{
			uint8_t i;
			for (i = 0; i != 3; ++i) {
				bulk_out_packets[i].num_iovs = 1;
				bulk_out_packets[i].iovs = bulk_out_iovecs[i];
				bulk_out_iovecs[i][0].len = sizeof(bulk_out_headers[i]);
				bulk_out_iovecs[i][0].ptr = &bulk_out_headers[i];
				bulk_out_headers[i].sequence_flags.first_micropacket = 0;
				bulk_out_headers[i].sequence_flags.last_micropacket = 0;
			}
		}
		now = (UFRMH << 8) | UFRML;



		/* Send state transport micropackets. */
		if (dongle_status.robots) {
			/* Assemble the packets. */
			CRITSEC_ENTER(cs);
			for (i = 1, j = 0, mask = 1 << 1; i != 16; ++i, mask <<= 1) {
				if (dongle_status.robots & mask) {
					drive_micropackets[j].pipe_robot.pipe = PIPE_DRIVE;
					drive_micropackets[j].pipe_robot.robot = i;
					memcpyram2ram(drive_micropackets[j].payload, state_transport_out_drive[i - 1], STATE_TRANSPORT_OUT_DRIVE_SIZE);
					++j;
				}
			}
			CRITSEC_LEAVE(cs);

			/* Submit the packets to the XBee layer. */
			if (j != 0) {
				i = j;
				if (i > 8) {
					i = 8;
				}
				drive_iovecs[0][0].len = sizeof(DRIVE_XBEE_HEADER);
				drive_iovecs[0][0].ptr = &DRIVE_XBEE_HEADER;
				drive_iovecs[0][1].len = 1;
				/* Determine if we should poll a robot.
				 * We poll a robot if:
				 * (1) There is no previous inbound packet outstanding, OR
				 * (2) The timeout for the previous inbound packet expired. */
				if (!in_pending || ((now - last_poll_time) & 0x3FF) > POLL_TIMEOUT) {
					do {
						if (pollee == 15) {
							pollee = 1;
						} else {
							++pollee;
						}
					} while (!(dongle_status.robots & (((uint16_t) 1) << pollee)));
					drive_iovecs[0][1].ptr = &pollee;
					in_pending = true;
					last_poll_time = now;
				} else {
					drive_iovecs[0][1].ptr = &ZERO;
				}
				drive_iovecs[0][2].len = i * (STATE_TRANSPORT_OUT_DRIVE_SIZE + 2);
				drive_iovecs[0][2].ptr = drive_micropackets;
				drive_packets[0].num_iovs = 3;
				drive_packets[0].iovs = drive_iovecs[0];
				xbee_txpacket_queue(&drive_packets[0], 0);
				++num_drive_sent;
				if (j > 8) {
					i = j - 8;
					drive_iovecs[1][0].len = sizeof(DRIVE_XBEE_HEADER);
					drive_iovecs[1][0].ptr = &DRIVE_XBEE_HEADER;
					drive_iovecs[1][1].len = 1;
					drive_iovecs[1][1].ptr = &ZERO;
					drive_iovecs[1][2].len = i * (STATE_TRANSPORT_OUT_DRIVE_SIZE + 2);
					drive_iovecs[1][2].ptr = drive_micropackets + 8;
					drive_packets[1].num_iovs = 3;
					drive_packets[1].iovs = drive_iovecs[1];
					xbee_txpacket_queue(&drive_packets[1], 0);
					++num_drive_sent;
				}
			}
		}



		/* Send up to three interrupt packets. */
		while (num_interrupt_out_sent != 3) {
			uint8_t robot, pipe;
			if (!(interrupt_out_packet = interrupt_out_get())) {
				/* No more interrupt packets to send. */
				break;
			}
			robot = interrupt_out_packet->buffer[0] >> 4;
			pipe = interrupt_out_packet->buffer[0] & 0x0F;
			if (pipe_info[robot - 1][pipe].used) {
				/* A message has already been sent to this pipe on this robot.
				 * To avoid sequence number problems, do not send a second message in the same cycle.
				 * Just stop here. */
				interrupt_out_unget(interrupt_out_packet);
				break;
			}
			if (!(dongle_status.robots & (((uint16_t) 1) << robot))) {
				/* This robot is offline.
				 * Report an error and drop the packet. */
				local_error_queue_add(40 + robot - 1);
				interrupt_out_free(interrupt_out_packet);
				continue;
			}
			if (interrupt_out_packet->cookie == 0xFF) {
				/* The message has no sequence number. Assign one. */
				interrupt_out_packet->cookie = pipe_info[robot - 1][pipe].sequence++;
			}

			/* Keep a record of the packet. */
			interrupt_out_sent[num_interrupt_out_sent] = interrupt_out_packet;

			/* Construct a header. */
			interrupt_out_headers[num_interrupt_out_sent].xbee_header.frame_id = next_frame;
			if (!++next_frame) {
				next_frame = 1;
			}
			interrupt_out_headers[num_interrupt_out_sent].xbee_header.address_low = 0x20 | robot;
			interrupt_out_headers[num_interrupt_out_sent].pipe = pipe;
			interrupt_out_headers[num_interrupt_out_sent].sequence = interrupt_out_packet->cookie;

			/* Assemble and queue the packet. */
			interrupt_out_iovecs[num_interrupt_out_sent][0].len = sizeof(interrupt_header_t);
			interrupt_out_iovecs[num_interrupt_out_sent][0].ptr = &interrupt_out_headers[num_interrupt_out_sent];
			interrupt_out_iovecs[num_interrupt_out_sent][1].len = interrupt_out_packet->length - 1;
			interrupt_out_iovecs[num_interrupt_out_sent][1].ptr = interrupt_out_packet->buffer + 1;
			interrupt_out_packets[num_interrupt_out_sent].num_iovs = 2;
			interrupt_out_packets[num_interrupt_out_sent].iovs = interrupt_out_iovecs[num_interrupt_out_sent];
			xbee_txpacket_queue(&interrupt_out_packets[num_interrupt_out_sent], 0);
			++num_interrupt_out_sent;
		}



		/* If we need to skip the rest of a faulty bulk message, do that. */
		while (bulk_skip_message && (bulk_out_packet = bulk_out_get())) {
			if (bulk_out_packet->length != sizeof(bulk_out_packet->buffer)) {
				bulk_skip_message = false;
			}
			bulk_out_free(bulk_out_packet);
		}

		/* Send up to three bulk packets, minus the number of interrupt packets already sent. */
		if (num_interrupt_out_sent != 3) {
			uint8_t next_sent_usb_packet = 0;
			uint8_t bytes_in_current_micropacket = 0;
			uint8_t robot = bulk_out_current_recipient >> 4;
			uint8_t pipe = bulk_out_current_recipient & 0x0F;
			uint8_t byte_count;
			uint8_t is_first;
			for (;;) {
				bulk_out_packet = bulk_out_get();
				if (!bulk_out_packet) {
					/* No more packets available yet. */
					if (bytes_in_current_micropacket) {
						/* We have already put some bytes in the current packet.
						 * We need to send the partially-filled packet. */
						bulk_out_micropacket_lengths[num_bulk_out_sent] = bytes_in_current_micropacket;
						robot = bulk_out_current_recipient >> 4;
						pipe = bulk_out_current_recipient & 0x0F;
						bulk_out_headers[num_bulk_out_sent].xbee_header.frame_id = next_frame;
						if (!++next_frame) {
							next_frame = 1;
						}
						bulk_out_headers[num_bulk_out_sent].xbee_header.address_low = 0x20 | robot;
						bulk_out_headers[num_bulk_out_sent].pipe = pipe;
						bulk_out_headers[num_bulk_out_sent].sequence_flags.sequence = pipe_info[robot - 1][pipe].sequence++;
						xbee_txpacket_queue(&bulk_out_packets[num_bulk_out_sent], 0);
						++num_bulk_out_sent;
					}
					/* We're done here. */
					break;
				}

				if (!bulk_out_current_recipient) {
					/* This is the first USB packet in the message. */
					if (!bulk_out_packet->length) {
						/* The host for no apparent reason sent a completely empty bulk transfer.
						 * Ignore it. */
						bulk_out_free(bulk_out_packet);
						continue;
					}
					/* Extract the recipient information. */
					bulk_out_current_recipient = bulk_out_packet->buffer[0];
					robot = bulk_out_current_recipient >> 4;
					pipe = bulk_out_current_recipient & 0x0F;
					if (!robot || !((1 << pipe) & pipe_out_mask & pipe_bulk_mask)) {
						/* The host sent a bulk message to an invalid robot or pipe.
						 * Queue an error and ignore the rest of the message. */
						local_error_queue_add(38);
						bulk_out_current_recipient = 0;
						if (bulk_out_packet->length == sizeof(bulk_out_packet->buffer)) {
							bulk_skip_message = true;
						}
						bulk_out_free(bulk_out_packet);
						break;
					}
					/* Mark the micropacket as being the first in the message. */
					bulk_out_headers[0].sequence_flags.first_micropacket = 1;
					is_first = 1;
				} else {
					is_first = 0;
				}

				/* Record the packet. */
				bulk_out_sent_usb_packets[next_sent_usb_packet] = bulk_out_packet;
				++next_sent_usb_packet;

				/* Put some data in the current XBee packet. */
				byte_count = 100 - (sizeof(bulk_header_t) - sizeof(xbee_tx16_header_t)) - bytes_in_current_micropacket;
				if (byte_count > bulk_out_packet->length - bulk_out_packet->cookie - is_first) {
					byte_count = bulk_out_packet->length - bulk_out_packet->cookie - is_first;
				}
				bulk_out_iovecs[num_bulk_out_sent][bulk_out_packets[num_bulk_out_sent].num_iovs].len = byte_count;
				bulk_out_iovecs[num_bulk_out_sent][bulk_out_packets[num_bulk_out_sent].num_iovs].ptr = bulk_out_packet->buffer + bulk_out_packet->cookie + is_first;
				++bulk_out_packets[num_bulk_out_sent].num_iovs;
				bytes_in_current_micropacket += byte_count;

				if (bulk_out_packet->length != sizeof(bulk_out_packet->buffer) && bulk_out_packet->cookie + is_first + byte_count == bulk_out_packet->length) {
					/* This USB packet is short and therefore indicates the end of the bulk message.
					 * Also, the USB packet was able to fit entirely in the current micropacket.
					 * Therefore, this micropacket is the end of the message. */
					bulk_out_headers[num_bulk_out_sent].sequence_flags.last_micropacket = 1;
					/* Send the packet. */
					bulk_out_micropacket_lengths[num_bulk_out_sent] = bytes_in_current_micropacket;
					robot = bulk_out_current_recipient >> 4;
					pipe = bulk_out_current_recipient & 0x0F;
					bulk_out_headers[num_bulk_out_sent].xbee_header.frame_id = next_frame;
					if (!++next_frame) {
						next_frame = 1;
					}
					bulk_out_headers[num_bulk_out_sent].xbee_header.address_low = 0x20 | robot;
					bulk_out_headers[num_bulk_out_sent].pipe = pipe;
					bulk_out_headers[num_bulk_out_sent].sequence_flags.sequence = pipe_info[robot - 1][pipe].sequence++;
					xbee_txpacket_queue(&bulk_out_packets[num_bulk_out_sent], 0);
					++num_bulk_out_sent;
					/* We're done here. */
					break;
				}

				if (bytes_in_current_micropacket == 100 - (sizeof(bulk_header_t) - sizeof(xbee_tx16_header_t))) {
					/* This XBee packet is full. Send it. */
					bulk_out_micropacket_lengths[num_bulk_out_sent] = bytes_in_current_micropacket;
					robot = bulk_out_current_recipient >> 4;
					pipe = bulk_out_current_recipient & 0x0F;
					bulk_out_headers[num_bulk_out_sent].xbee_header.frame_id = next_frame;
					if (!++next_frame) {
						next_frame = 1;
					}
					bulk_out_headers[num_bulk_out_sent].xbee_header.address_low = 0x20 | robot;
					bulk_out_headers[num_bulk_out_sent].pipe = pipe;
					bulk_out_headers[num_bulk_out_sent].sequence_flags.sequence = pipe_info[robot - 1][pipe].sequence++;
					xbee_txpacket_queue(&bulk_out_packets[num_bulk_out_sent], 0);
					++num_bulk_out_sent;
					bytes_in_current_micropacket = 0;
					if (num_bulk_out_sent + num_interrupt_out_sent == 3) {
						/* We've reached the packet limit.
						 * We're done here. */
						break;
					}
				}

				if (bulk_out_packet->cookie + is_first + byte_count != bulk_out_packet->length) {
					/* There is some more data left in the USB packet that didn't fit into the original XBee packet.
					 * We are now starting up a new XBee packet.
					 * Put the data in there. */
					bulk_out_iovecs[num_bulk_out_sent][1].len = bulk_out_packet->length - (bulk_out_packet->cookie + is_first + byte_count);
					bulk_out_iovecs[num_bulk_out_sent][1].ptr = bulk_out_packet->buffer + bulk_out_packet->cookie + is_first + byte_count;
					++bulk_out_packets[num_bulk_out_sent].num_iovs;
					bytes_in_current_micropacket += bulk_out_iovecs[num_bulk_out_sent][1].len;

					if (bulk_out_packet->length != sizeof(bulk_out_packet->buffer)) {
						/* This USB packet is short and therefore indicates the end of the bulk message.
						 * Therefore, this micropacket is the end of the message. */
						bulk_out_headers[num_bulk_out_sent].sequence_flags.last_micropacket = 1;
						/* Send the packet. */
						bulk_out_micropacket_lengths[num_bulk_out_sent] = bytes_in_current_micropacket;
						robot = bulk_out_current_recipient >> 4;
						pipe = bulk_out_current_recipient & 0x0F;
						bulk_out_headers[num_bulk_out_sent].xbee_header.frame_id = next_frame;
						if (!++next_frame) {
							next_frame = 1;
						}
						bulk_out_headers[num_bulk_out_sent].xbee_header.address_low = 0x20 | robot;
						bulk_out_headers[num_bulk_out_sent].pipe = pipe;
						bulk_out_headers[num_bulk_out_sent].sequence_flags.sequence = pipe_info[robot - 1][pipe].sequence++;
						xbee_txpacket_queue(&bulk_out_packets[num_bulk_out_sent], 0);
						++num_bulk_out_sent;
						/* We're done here. */
						break;
					}
				}
			}
		}



		if (dongle_status.robots != 0xFFFE && !--discovery_counter) {
			/* Send a discovery packet. */
			do {
				if (discovery_robot == 15) {
					discovery_robot = 1;
				} else {
					++discovery_robot;
				}
			} while (dongle_status.robots & (((uint16_t) 1) << discovery_robot));
			discovery_header.xbee_header.frame_id = next_frame;
			if (!++next_frame) {
				next_frame = 1;
			}
			discovery_header.xbee_header.address_low = 0x20 | discovery_robot;
			discovery_iovec.len = sizeof(discovery_header) + 1;
			discovery_iovec.ptr = &discovery_header;
			discovery_packet.num_iovs = 1;
			discovery_packet.iovs = &discovery_iovec;
			xbee_txpacket_queue(&discovery_packet, 0);
			discovery_sent = true;
			discovery_counter = 5;
		} else {
			/* Not time to do discovery right now. */
			discovery_sent = false;
		}



		/* Wait for all the XBee packets to finish being pushed over the serial port. */
		{
			uint8_t num_packets = num_drive_sent + num_interrupt_out_sent + num_bulk_out_sent;
			if (discovery_sent) {
				++num_packets;
			}
			while (num_packets) {
				if (xbee_txpacket_dequeue()) {
					--num_packets;
				}
				check_idle();
			}
		}



		/* Process the transmit status packets and any data received from a robot. */
		{
			uint8_t bulk_out_success_mask = 0;
			{
				uint8_t num_packets = num_interrupt_out_sent + num_bulk_out_sent;
				__data xbee_rxpacket_t *pkt;
				uint8_t i;
				if (discovery_sent) {
					++num_packets;
				}
				while (num_packets && ((((UFRMH << 8) | UFRML) - now) & 0x3FF) < 1000) {
					if (pkt = xbee_rxpacket_get()) {
						if (pkt->xbee == 0) {
							/* This packet arrived on XBee 0.
							 * It should be a transmit status packet. */
							if (pkt->ptr[0] == XBEE_API_ID_TX_STATUS) {
								/* Figure out what's going on based on the frame ID. */
								for (i = 0; i != num_interrupt_out_sent; ++i) {
									if (pkt->ptr[1] == interrupt_out_headers[i].xbee_header.frame_id) {
										if (pkt->ptr[2] == 0) {
											/* Transmission successful.
											 * Free the USB packet. */
											interrupt_out_free(interrupt_out_sent[i]);
											interrupt_out_sent[i] = 0;
										} else {
											/* Transmission failed.
											 * Count the failure. */
											++comm_failures[bulk_out_headers[i].xbee_header.address_low & 0x0F];
											/* Requeue the USB packet. */
											interrupt_out_unget(interrupt_out_sent[i]);
											interrupt_out_sent[i] = 0;
										}
										--num_packets;
									}
								}
								for (i = 0; i != num_bulk_out_sent; ++i) {
									if (pkt->ptr[1] == bulk_out_headers[i].xbee_header.frame_id) {
										if (pkt->ptr[2] == 0) {
											/* Transmission successful.
											 * Mark it in the mask. */
											bulk_out_success_mask |= 1 << i;
										}
										--num_packets;
									}
								}
								if (discovery_sent) {
									if (pkt->ptr[1] == discovery_header.xbee_header.frame_id) {
										if (pkt->ptr[2] == 0) {
											/* Transmission successful.
											 * This robot is now alive. */
											dongle_status.robots |= ((uint16_t) 1) << discovery_robot;
											dongle_status_dirty();
										}
										--num_packets;
									}
								}
							}
						} else {
							/* This packet arrived on XBee 1.
							 * It should be a receive data packet. */
							if (pkt->ptr[0] == XBEE_API_ID_RX16 && pkt->ptr[1] == 0x7B && (pkt->ptr[2] & 0xF0) == 0x30 && pkt->ptr[2] != 0x30) {
								uint8_t robot = pkt->ptr[2] & 0x0F;
								__data const uint8_t *ptr = pkt->ptr + 5;
								uint8_t len = pkt->len - 5;
								while (len) {
									if (ptr[0] > len || (ptr[1] & 0xF0) != 0) {
										/* Micropacket overflows packet. */
										local_error_queue_add(55 + robot - 1);
										break;
									} else {
										switch (ptr[1] & 0x0F) {
											case PIPE_FEEDBACK:
												if (len == STATE_TRANSPORT_IN_FEEDBACK_SIZE + 2) {
													memcpyram2ram(state_transport_in_feedback[robot - 1], ptr + 2, STATE_TRANSPORT_IN_FEEDBACK_SIZE);
													state_transport_in_feedback_dirty(robot);
												} else {
													local_error_queue_add(55 + robot - 1);
												}
												break;

											default:
												local_error_queue_add(55 + robot - 1);
												break;
										}
										len -= *ptr;
										ptr += *ptr;
									}
								}
							}
						}
						xbee_rxpacket_free(pkt);
					}
				}
				if (num_packets) {
					local_error_queue_add(21);
				}
			}

			if (num_bulk_out_sent) {
				{
					__data bulk_out_packet_t *pkt;
					uint8_t i, j, to_consume;
					uint8_t current_usb_packet = 0;

					/* First, permanently consume bytes from USB packets whose XBee packets were successful. */
					for (i = 0; bulk_out_success_mask & (1 << i); ++i) {
						pkt = bulk_out_sent_usb_packets[current_usb_packet];
						if (bulk_out_headers[i].sequence_flags.first_micropacket) {
							/* This is the first USB packet in the message, so consume the header. */
							++pkt->cookie;
						}
						if (bulk_out_headers[i].sequence_flags.last_micropacket) {
							/* This is the last packet in the message, so clear the current recipient. */
							bulk_out_current_recipient = 0;
						}
						j = 0;
						while (j != bulk_out_micropacket_lengths[i]) {
							/* Consume some payload bytes. */
							to_consume = bulk_out_micropacket_lengths[i] - j;
							if (to_consume > pkt->length - pkt->cookie) {
								to_consume = pkt->length - pkt->cookie;
							}
							pkt->cookie += to_consume;
							j += to_consume;

							if (pkt->cookie == pkt->length) {
								/* This USB packet is completely finished.
								 * Move to the next one. */
								++current_usb_packet;
								pkt = bulk_out_sent_usb_packets[current_usb_packet];
							}
						}
					}
				}

				if (bulk_out_success_mask != (1 << num_bulk_out_sent) - 1) {
					uint8_t i;

					/* Some bulk packets were unsuccessful. */
					for (i = 0; !(bulk_out_success_mask & (1 << i)); ++i);

					/* Adjust the sequence number counter so we will reuse the sequence number for this packet. */
					pipe_info[(bulk_out_current_recipient >> 4) - 1][bulk_out_current_recipient & 0x0F].sequence = bulk_out_headers[i].sequence_flags.sequence;

					if (bulk_out_headers[i].sequence_flags.first_micropacket) {
						/* The first micropacket of the message failed.
						 * Therefore, the USB packet containing the header has not really had the header consumed.
						 * Signal this appropriately. */
						bulk_out_current_recipient = 0;
					}
				}

				{
					uint8_t i = sizeof(bulk_out_sent_usb_packets) / sizeof(*bulk_out_sent_usb_packets) - 1;
					do {
						if (bulk_out_sent_usb_packets[i]) {
							if (bulk_out_sent_usb_packets[i]->cookie == bulk_out_sent_usb_packets[i]->length) {
								/* This USB packet is completely finished. Free it. */
								bulk_out_free(bulk_out_sent_usb_packets[i]);
							} else {
								/* This USB packet still has more data to send. Keep it. */
								bulk_out_unget(bulk_out_sent_usb_packets[i]);
							}
						}
					} while (i--);
				}
			}
		}
	}
}

