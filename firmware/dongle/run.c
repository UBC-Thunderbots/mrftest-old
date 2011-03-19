#include "run.h"
#include "critsec.h"
#include "debug.h"
#include "dongle_status.h"
#include "error_reporting.h"
#include "global.h"
#include "interrupt_in.h"
#include "interrupt_out.h"
#include "pipes.h"
#include "queue.h"
#include "state_transport_in.h"
#include "state_transport_out.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define POLL_TIMEOUT 25
#define COMM_FAILURE_LIMIT 50

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
	drive_block_t payload;
} drive_micropacket_t;

typedef struct {
	xbee_tx16_header_t xbee_header;
	uint8_t pipe;
	uint8_t sequence;
} interrupt_header_t;

typedef struct {
	xbee_tx16_header_t xbee_header;
	uint8_t flag;
} discovery_header_t;

void run(void) {
	static const uint8_t FE = 0xFE;	
	static const xbee_tx16_header_t DRIVE_XBEE_HEADER = { XBEE_API_ID_TX16, 0x00, 0xFF, 0xFF, XBEE_TX16_OPTION_DISABLE_ACK };
	uint8_t num_drive_sent;
	static drive_micropacket_t drive_micropackets[16];
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

	uint8_t discovery_counter = 5, discovery_robot = 0;
	static discovery_header_t discovery_header;
	static xbee_txpacket_iovec_t discovery_iovec;
	static xbee_txpacket_t discovery_packet;
	BOOL discovery_sent;

	uint16_t now, mask;
	uint8_t next_frame = 1;
	static uint8_t comm_failures[16];
	static pipe_info_t pipe_info[16][PIPE_MAX + 1];
	CRITSEC_DECLARE(cs);

	/* Clear state. */
	{
		uint8_t i;
		for (i = 0; i != 16; ++i) {
			drive_micropackets[i].micropacket_length = sizeof(*drive_micropackets);
		}
		for (i = 0; i != 3; ++i) {
			interrupt_out_headers[i].xbee_header.api_id = XBEE_API_ID_TX16;
			interrupt_out_headers[i].xbee_header.address_high = 0x7B;
			interrupt_out_headers[i].xbee_header.options = 0;
		}
	}
	discovery_header.xbee_header.api_id = XBEE_API_ID_TX16;
	discovery_header.xbee_header.address_high = 0x7B;
	discovery_header.xbee_header.options = 0;
	discovery_header.flag = 0xFF;
	memset(comm_failures, 0, sizeof(comm_failures));

	while (!should_shut_down) {
		/* Initialize per-cycle state. */
		num_drive_sent = 0;
		num_interrupt_out_sent = 0;
		now = (UFRMH << 8) | UFRML;



		/* Check for a timeout on a prior poll. */
		if (in_pending && ((now - last_poll_time) & 0x3FF) > POLL_TIMEOUT) {
			if (dongle_status.robots & (1 << pollee)) {
				if (++comm_failures[pollee] == COMM_FAILURE_LIMIT) {
					comm_failures[pollee] = 0;
					dongle_status.robots &= ~(1 << pollee);
					dongle_status_dirty();
				}
			}
			in_pending = false;
		}



		/* Send state transport micropackets. */
		if (dongle_status.robots) {
			uint8_t i, j;
			/* Assemble the packets. */
			CRITSEC_ENTER_LOW(cs);
			for (i = 0, j = 0, mask = 1 << 0; i != 16; ++i, mask <<= 1) {
				if (dongle_status.robots & mask) {
					drive_micropackets[j].pipe_robot.pipe = PIPE_DRIVE;
					drive_micropackets[j].pipe_robot.robot = i;
					memcpyram2ram(&drive_micropackets[j].payload, &state_transport_out_drive[i], sizeof(drive_block_t));
					if (dongle_status.estop != ESTOP_STATE_RUN) {
						drive_micropackets[j].payload.flags.enable_robot = 0;
					}
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
				/* Determine if we should poll a robot. */
				if (!in_pending) {
					do {
						pollee = (pollee + 1) & 15;
					} while (!(dongle_status.robots & (((uint16_t) 1) << pollee)));
					drive_iovecs[0][1].ptr = &pollee;
					in_pending = true;
					last_poll_time = now;
				} else {
					drive_iovecs[0][1].ptr = &FE;
				}
				drive_iovecs[0][2].len = i * (sizeof(drive_block_t) + 2);
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
					drive_iovecs[1][1].ptr = &FE;
					drive_iovecs[1][2].len = i * (sizeof(drive_block_t) + 2);
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
			if (pipe_info[robot][pipe].used) {
				/* A message has already been sent to this pipe on this robot.
				 * To avoid sequence number problems, do not send a second message in the same cycle.
				 * Just stop here. */
				interrupt_out_unget(interrupt_out_packet);
				break;
			}
			if (!(dongle_status.robots & (((uint16_t) 1) << robot))) {
				/* This robot is offline.
				 * Report an error and drop the packet. */
				error_reporting_add(FAULT_SEND_FAILED_ROBOT0 + robot);
				interrupt_out_free(interrupt_out_packet);
				continue;
			}
			if (interrupt_out_packet->cookie == 0xFF) {
				/* The message has no sequence number. Assign one. */
				interrupt_out_packet->cookie = pipe_info[robot][pipe].sequence++;
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



		if (dongle_status.robots != 0xFFFF && !--discovery_counter) {
			/* Send a discovery packet. */
			do {
				discovery_robot = (discovery_robot + 1) & 15;
			} while (dongle_status.robots & (((uint16_t) 1) << discovery_robot));
			discovery_header.xbee_header.frame_id = next_frame;
			if (!++next_frame) {
				next_frame = 1;
			}
			discovery_header.xbee_header.address_low = 0x20 | discovery_robot;
			discovery_iovec.len = sizeof(discovery_header);
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
			uint8_t num_packets = num_drive_sent + num_interrupt_out_sent;
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
				uint8_t num_packets = num_interrupt_out_sent;
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
							if (pkt->buf[0] == XBEE_API_ID_TX_STATUS) {
								/* Figure out what's going on based on the frame ID. */
								for (i = 0; i != num_interrupt_out_sent; ++i) {
									if (pkt->buf[1] == interrupt_out_headers[i].xbee_header.frame_id) {
										if (pkt->buf[2] == 0) {
											/* Transmission successful.
											 * Free the USB packet. */
											interrupt_out_free(interrupt_out_sent[i]);
											interrupt_out_sent[i] = 0;
										} else {
											/* Transmission failed.
											 * Count the failure. */
											uint8_t robot = interrupt_out_headers[i].xbee_header.address_low & 0x0F;
											if (dongle_status.robots & (1 << robot)) {
												if (++comm_failures[robot] == COMM_FAILURE_LIMIT) {
													comm_failures[robot] = 0;
													dongle_status.robots &= ~(1 << robot);
													dongle_status_dirty();
												}
											}
											/* Requeue the USB packet. */
											interrupt_out_unget(interrupt_out_sent[i]);
											interrupt_out_sent[i] = 0;
										}
										--num_packets;
									}
								}
								if (discovery_sent) {
									if (pkt->buf[1] == discovery_header.xbee_header.frame_id) {
										if (pkt->buf[2] == 0) {
											/* Transmission successful.
											 * This robot is now alive. */
											dongle_status.robots |= ((uint16_t) 1) << discovery_robot;
											dongle_status_dirty();
											/* Clear sequence numbers. */
											{
												uint8_t i;
												for (i = 0; i != PIPE_MAX + 1; ++i) {
													if (PIPE_OUT_MASK & (1 << i)) {
														pipe_info[discovery_robot][i].used = 0;
														pipe_info[discovery_robot][i].sequence = 0;
													} else {
														pipe_info[discovery_robot][i].used = 0;
														pipe_info[discovery_robot][i].sequence = 63;
													}
												}
											}
										}
										--num_packets;
									}
								}
							}
						} else {
							/* This packet arrived on XBee 1.
							 * It should be a receive data packet. */
							if (pkt->buf[0] == XBEE_API_ID_RX16 && pkt->buf[1] == 0x7B && (pkt->buf[2] & 0xF0) == 0x30) {
								uint8_t robot = pkt->buf[2] & 0x0F;
								__data uint8_t *ptr = pkt->buf + 5;
								uint8_t len = pkt->len - 5;
								uint16_t diff = (now - last_poll_time) & 0x3FF;
								if (robot == pollee) {
									in_pending = false;
									comm_failures[robot] = 0;
								}
								while (len) {
									if (ptr[0] < 2) {
										/* Micropacket too short to hold its own header. */
										error_reporting_add(FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT0 + robot);
										break;
									} else if (ptr[0] > len) {
										/* Micropacket overflows packet. */
										error_reporting_add(FAULT_IN_MICROPACKET_OVERFLOW_ROBOT0 + robot);
										break;
									} else if ((ptr[1] & 0xF0) != 0) {
										/* Micropacket directed to someone other than this dongle. */
										error_reporting_add(FAULT_IN_MICROPACKET_NOPIPE_ROBOT0 + robot);
										break;
									} else {
										uint8_t pipe = ptr[1] & 0x0F;
										if (pipe == PIPE_FEEDBACK) {
											if (ptr[0] == sizeof(feedback_block_t) + 2) {
												memcpyram2ram(state_transport_in_feedback[robot], ptr + 2, sizeof(feedback_block_t));
												state_transport_in_feedback_dirty(robot);
											} else {
												error_reporting_add(FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT0 + robot);
											}
										} else if ((1 << pipe) & PIPE_IN_MASK & PIPE_INTERRUPT_MASK) {
											if (ptr[0] >= 3) {
												if (ptr[2] != pipe_info[robot][pipe].sequence) {
													pipe_info[robot][pipe].sequence = ptr[2];
													ptr[2] = (robot << 4) | pipe;
													interrupt_in_send(ptr + 2, ptr[0] - 2);
												}
											} else {
												error_reporting_add(FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT0 + robot);
											}
										} else {
											error_reporting_add(FAULT_IN_MICROPACKET_NOPIPE_ROBOT0 + robot);
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
					error_reporting_add(FAULT_XBEE0_TIMEOUT);
				}
			}
		}
	}
}

