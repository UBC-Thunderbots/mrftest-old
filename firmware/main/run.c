#include "run.h"
#include "crc.h"
#include "drive.h"
#include "error_reporting.h"
#include "feedback.h"
#include "fw.h"
#include "leds.h"
#include "params.h"
#include "parbus.h"
#include "pins.h"
#include "pipes.h"
#include "spi.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <delay.h>
#include <pic18fregs.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * \brief The XBee API ID for a transmit data packet.
 */
#define XBEE_API_ID_TX_16 0x01

/**
 * \brief The XBee API ID for a transmit status packet.
 */
#define XBEE_API_ID_TX_STATUS 0x89

/**
 * \brief The XBee API ID for a receive data packet.
 */
#define XBEE_API_ID_RX_16 0x81

/**
 * \brief The number of control loop iterations before the drive pipe is considered to have timed out.
 */
#define DRIVE_TIMEOUT_LIMIT 200

/**
 * \brief The type of an XBee transmit header.
 */
typedef struct {
	uint8_t api_id;
	uint8_t frame_id;
	uint8_t address_high;
	uint8_t address_low;
	uint8_t options;
} xbee_tx16_header_t;

/**
 * \brief The possible states we can be in with respect to an inbound packet we are trying to send.
 */
typedef enum {
	/**
	 * \brief We are not trying to do anything with an inbound packet (any packet we might have sent has been delivered successfully).
	 */
	INBOUND_STATE_IDLE,

	/**
	 * \brief We are waiting for the inbound packet to be sent over the serial line to the XBee.
	 */
	INBOUND_STATE_SENDING,

	/**
	 * \brief We are waiting for XBee #1 to send us a transmit status packet.
	 */
	INBOUND_STATE_AWAITING_STATUS,

	/**
	 * \brief We are waiting for another poll because the packet could not be delivered and we need to try again.
	 */
	INBOUND_STATE_AWAITING_POLL,
} inbound_state_t;

void run(void) {
	__data xbee_rxpacket_t *rxpacket;
	__data const uint8_t *rxptr;
	static xbee_tx16_header_t txheader = { XBEE_API_ID_TX_16, 0x00, 0x7B, 0x30, 0x00 };
	static xbee_txpacket_iovec_t txiovs[4], txiovs_shadow[4];
	static xbee_txpacket_t txpkt, txpkt_shadow;
	inbound_state_t inbound_state = INBOUND_STATE_IDLE;
	static const uint8_t FEEDBACK_MICROPACKET_HEADER[2] = { 2 + sizeof(feedback_block_t), PIPE_FEEDBACK };
	static feedback_block_t txpkt_feedback_shadow;
	static uint8_t sequence[PIPE_MAX + 1];
	static firmware_response_t firmware_response;
	BOOL firmware_response_pending = false;
	enum {
		REBOOT_PENDING_NO,
		REBOOT_PENDING_NEXT_PACKET,
		REBOOT_PENDING_THIS_PACKET,
	} reboot_pending = REBOOT_PENDING_NO;
	BOOL flash_page_program_active = false, flash_burn_active = false, flash_reburn_params = false;
	static params_t flash_temp_params;
	static parbus_txpacket_t parbus_txpacket;
	static parbus_rxpacket_t parbus_rxpacket;
	uint8_t drive_timeout = 0;
	uint16_t kick_power = 0;
	const uint8_t robot_number = params.robot_number;

	/* Clear state. */
	memset(sequence, 0, sizeof(sequence));
	txiovs[0].len = sizeof(txheader);
	txiovs[0].ptr = &txheader;
	txiovs[1].len = sizeof(FEEDBACK_MICROPACKET_HEADER);
	txiovs[1].ptr = FEEDBACK_MICROPACKET_HEADER;
	txiovs[2].len = sizeof(txpkt_feedback_shadow);
	txiovs[2].ptr = &txpkt_feedback_shadow;
	txpkt.iovs = txiovs;
	memset(&parbus_txpacket, 0, sizeof(parbus_txpacket));
	memset(&parbus_rxpacket, 0, sizeof(parbus_rxpacket));

	/* Start running a 200Hz control loop timer.
	 *        /-------- 8-bit reads and writes
	 *        |/------- Timer 1 is not driving system clock
	 *        ||//----- 1:1 prescale
	 *        ||||/---- Timer 1 oscillator disabled
	 *        |||||/--- Ignored
	 *        ||||||/-- Internal clock
	 *        |||||||/- Timer enabled */
	T1CON = 0b00000001;
	/*          ////----- Ignored
	 *          ||||////- Compare mode with special event trigger on match */
	CCP1CON = 0b00001011;
	CCPR1H = 60000 / 256;
	CCPR1L = 60000 % 256;

	/* Perform a calibration ADC reading.
	 *         |/------- Execute calibration on next acquisition
	 *         ||///---- Set acquisition time 4TAD = 5⅓µs
	 *         |||||///- Set conversion clock TAD = Fosc/64 = 1⅓µs */
	ADCON1 = 0b11010110;
	/*         /-------- Negative reference is AVSS
	 *         |/------- Positive reference is AVDD
	 *         ||////--- Channel 0
	 *         ||||||/-- Do not start acquisition yet
	 *         |||||||/- ADC on */
	ADCON0 = 0b00000001;
	ADCON0bits.GO = 1;
	while (ADCON0bits.GO);
	ADCON1bits.ADCAL = 0;

	/* Start a conversion of the battery voltage. */
	ADCON0bits.GO = 1;

	/* Run forever handling events. */
	for (;;) {
		if (xbee_txpacket_dequeue() == &txpkt_shadow) {
			/* An inbound finished being sent. */
			if (inbound_state == INBOUND_STATE_SENDING) {
				/* Update our current state. */
				inbound_state = INBOUND_STATE_AWAITING_STATUS;

				/* Start a timeout to deal with possible serial line failures. */
				TMR0H = 0;
				TMR0L = 0;
				INTCONbits.TMR0IF = 0;
			}
		}

		if (rxpacket = xbee_rxpacket_get()) {
			/* There's an XBee packet to deal with. */
			if (rxpacket->xbee == 0 && rxpacket->len > 5 && rxpacket->buf[0] == XBEE_API_ID_RX_16 && rxpacket->buf[1] == 0x7B && rxpacket->buf[2] == 0x20) {
				/* It's a receive data packet from XBee #0 with data from the dongle. */
				if (rxpacket->buf[4] & 0x02) {
					/* It's a broadcast packet and therefore contains a poll code and a list of state transport micropackets for multiple robots. */
					rxptr = rxpacket->buf + 6;
					while (rxptr - rxpacket->buf < rxpacket->len) {
						if (rxptr[0] >= 2) {
							if (rxptr[0] <= (rxpacket->len - (rxptr - rxpacket->buf))) {
								/* It's reasonably correctly structured. */
								uint8_t robot = rxptr[1] >> 4;
								uint8_t pipe = rxptr[1] & 0x0F;
								if (robot && robot == robot_number) {
									/* It's addressed to us. */
									if (pipe == PIPE_DRIVE) {
										if (rxptr[0] == 2 + sizeof(drive_block)) {
											/* It's new drive data. */
											memcpyram2ram(&drive_block, rxptr + 2, sizeof(drive_block));
											drive_timeout = DRIVE_TIMEOUT_LIMIT;
										} else {
											/* It's the wrong length. */
											error_reporting_add(FAULT_OUT_MICROPACKET_BAD_LENGTH);
										}
									} else {
										/* It's addressed to an unknown pipe. */
										error_reporting_add(FAULT_OUT_MICROPACKET_NOPIPE);
									}
								}
								rxptr += rxptr[0];
							} else {
								/* It overflows the end of the packet. */
								error_reporting_add(FAULT_OUT_MICROPACKET_OVERFLOW);
							}
						} else {
							/* It's an illegal length. */
							error_reporting_add(FAULT_OUT_MICROPACKET_BAD_LENGTH);
						}
					}

					if (rxpacket->buf[5] && rxpacket->buf[5] == robot_number && (inbound_state == INBOUND_STATE_IDLE || inbound_state == INBOUND_STATE_AWAITING_POLL)) {
						/* The poll code is asking us to send a packet. */
						if (inbound_state == INBOUND_STATE_AWAITING_POLL) {
							/* We should resend the last packet.
							 * The master copy should still be intact, so just shadow it and send it. */
							memcpyram2ram(txiovs_shadow, txiovs, sizeof(txiovs_shadow));
							txpkt_shadow.num_iovs = txpkt.num_iovs;
							txpkt_shadow.iovs = txiovs_shadow;
							/* We do, however, want a new frame number. */
							if (!++txheader.frame_id) {
								txheader.frame_id = 1;
							}
							xbee_txpacket_queue(&txpkt_shadow, 1);
						} else {
							/* Shadow the feedback block to avoid byte tearing if it's updated shortly. */
							memcpyram2ram(&txpkt_feedback_shadow, &feedback_block, sizeof(txpkt_feedback_shadow));
							txpkt.num_iovs = 3;
							if (firmware_response_pending) {
								/* A firmware response is pending.
								 * Add it to the packet. */
								txiovs[txpkt.num_iovs].len = firmware_response.micropacket_length;
								txiovs[txpkt.num_iovs].ptr = &firmware_response;
								++txpkt.num_iovs;
								firmware_response_pending = false;
							}
							/* Assign a frame number. */
							if (!++txheader.frame_id) {
								txheader.frame_id = 1;
							}
							/* The packet transmission layer will make a mess of our packet and IOVs, so shadow them. */
							memcpyram2ram(txiovs_shadow, txiovs, sizeof(txiovs_shadow));
							txpkt_shadow.num_iovs = txpkt.num_iovs;
							txpkt_shadow.iovs = txiovs_shadow;
							xbee_txpacket_queue(&txpkt_shadow, 1);
						}

						/* Mark state. */
						inbound_state = INBOUND_STATE_SENDING;
					}
				} else if (rxpacket->len == 6 && rxpacket->buf[5] == 0xFF) {
					/* It's a discovery and synchronization packet.
					 * Clear our sequence numbers. */
					memset(sequence, 0, sizeof(sequence));
				} else {
					/* It's a packet containing a single interrupt or bulk message. */
					if (rxpacket->buf[5] <= PIPE_MAX && ((1 << rxpacket->buf[5]) & pipe_out_mask & (pipe_interrupt_mask | pipe_bulk_mask))) {
						/* It's addressed to an existing interrupt or bulk pipe in the right direction. */
						if ((rxpacket->buf[6] & 63) == sequence[rxpacket->buf[5]]) {
							/* The sequence number is correct. */
							sequence[rxpacket->buf[5]] = (sequence[rxpacket->buf[5]] + 1) & 63;
							if (rxpacket->buf[5] == PIPE_FAULT_OUT) {
								/* The packet contains a fault clearing request. */
#warning implement
							} else if (rxpacket->buf[5] == PIPE_KICK) {
								/* The packet contains a kick request. */
								if (rxpacket->len == 10) {
									kick_power = rxpacket->buf[8] | (rxpacket->buf[9] << 8);
								} else {
									error_reporting_add(FAULT_OUT_MICROPACKET_BAD_LENGTH);
								}
							} else if (rxpacket->buf[5] == PIPE_FIRMWARE_OUT) {
								/* The packet contains a firmware request. */
								if (rxpacket->buf[6] & 0x80) {
									/* This is the start of a bulk message. */
									if (flash_page_program_active) {
										/* An earlier page program request was started but not finished.
										 * Abort it by manually pushing out *FOUR* (NOT a multiple of eight!) bits and then deasserting chip select. */
										SSP1CON1bits.SSPEN = 0;
										LAT_FLASH_CLK = 1;
										LAT_FLASH_CLK = 0;
										LAT_FLASH_CLK = 1;
										LAT_FLASH_CLK = 0;
										LAT_FLASH_CLK = 1;
										LAT_FLASH_CLK = 0;
										LAT_FLASH_CLK = 1;
										LAT_FLASH_CLK = 0;
										LAT_FLASH_CS = 1;
										SSP1CON1bits.SSPEN = 1;
										flash_page_program_active = false;
									}
									firmware_response_pending = false;
									switch (rxpacket->buf[7]) {
										case FIRMWARE_REQUEST_CHIP_ERASE:
											if (!params_load()) {
												error_reporting_add(FAULT_FLASH_PARAMS_CORRUPT);
											} else {
												/* Set write enable latch. */
												LAT_FLASH_CS = 0;
												spi_send(0x06);
												LAT_FLASH_CS = 1;

												/* Send the erase instruction. */
												LAT_FLASH_CS = 0;
												spi_send(0xC7);
												LAT_FLASH_CS = 1;

												/* Prepare a response, but do not queue it yet (we will do that when the operation finishes). */
												firmware_response.micropacket_length = sizeof(firmware_response) - sizeof(firmware_response.params);
												firmware_response.pipe = PIPE_FIRMWARE_IN;
												firmware_response.sequence = sequence[PIPE_FIRMWARE_IN];
												sequence[PIPE_FIRMWARE_IN] = (sequence[PIPE_FIRMWARE_IN] + 1) & 63;
												firmware_response.request = FIRMWARE_REQUEST_CHIP_ERASE;

												/* Remember that the operation is ongoing. */
												flash_burn_active = true;

												/* Remember that we will need to reburn the parameters block after the chip erase finishes. */
												flash_reburn_params = true;
											}
											break;

										case FIRMWARE_REQUEST_PAGE_PROGRAM:
											if (rxpacket->len >= 10) {
												uint8_t i;
												__data const uint8_t *p;

												/* Set write enable latch. */
												LAT_FLASH_CS = 0;
												spi_send(0x06);
												LAT_FLASH_CS = 1;

												/* Send the page program instruction. */
												LAT_FLASH_CS = 0;
												spi_send(0x02);
												spi_send(rxpacket->buf[9]);
												spi_send(rxpacket->buf[8]);
												spi_send(0);

												/* Send the data. */
												for (i = rxpacket->len - 10, p = rxpacket->buf + 10; i; --i, ++p) {
													spi_send(*p);
												}

												if (rxpacket->buf[6] & 0x40) {
													/* This packet is the end of the message.
													 * Terminate the write. */
													LAT_FLASH_CS = 1;

													/* Wait until complete. */
													LAT_FLASH_CS = 0;
													spi_send(0x05);
													while (spi_receive() & 0x01);
													LAT_FLASH_CS = 1;
												} else {
													/* This packet is not the end of the message.
													 * Leave chip select asserted.
													 * Mark state so next time we'll resume writing more bytes. */
													flash_page_program_active = true;
												}
											}
											break;

										case FIRMWARE_REQUEST_CRC_BLOCK:
											if ((rxpacket->buf[6] & 0x40) && rxpacket->len == 13) {
												uint16_t counter, crc = CRC16_EMPTY;

												/* Send the Read Data instruction and the address. */
												LAT_FLASH_CS = 0;
												spi_send(0x03);
												spi_send(rxpacket->buf[10]);
												spi_send(rxpacket->buf[9]);
												spi_send(rxpacket->buf[8]);

												/* Iterate the bytes. */
												counter = (rxpacket->buf[12] << 8) | rxpacket->buf[11];
												do {
													crc = crc_update(crc, spi_receive());
												} while (--counter);
												LAT_FLASH_CS = 1;

												/* Reply with the CRC. */
												firmware_response.micropacket_length = sizeof(firmware_response) - sizeof(firmware_response.params) + sizeof(firmware_response.params.compute_block_crc_params);
												firmware_response.pipe = PIPE_FIRMWARE_IN;
												firmware_response.sequence = sequence[PIPE_FIRMWARE_IN];
												sequence[PIPE_FIRMWARE_IN] = (sequence[PIPE_FIRMWARE_IN] + 1) & 63;
												firmware_response.request = FIRMWARE_REQUEST_CRC_BLOCK;
												firmware_response.params.compute_block_crc_params.address[0] = rxpacket->buf[8];
												firmware_response.params.compute_block_crc_params.address[1] = rxpacket->buf[9];
												firmware_response.params.compute_block_crc_params.address[2] = rxpacket->buf[10];
												firmware_response.params.compute_block_crc_params.length = (rxpacket->buf[12] << 8) | rxpacket->buf[11];
												firmware_response.params.compute_block_crc_params.crc = crc;
												firmware_response_pending = true;
											}
											break;

										case FIRMWARE_REQUEST_READ_PARAMS:
											firmware_response.micropacket_length = sizeof(firmware_response) - sizeof(firmware_response.params) + sizeof(firmware_response.params.operational_parameters);
											firmware_response.pipe = PIPE_FIRMWARE_IN;
											firmware_response.sequence = sequence[PIPE_FIRMWARE_IN];
											sequence[PIPE_FIRMWARE_IN] = (sequence[PIPE_FIRMWARE_IN] + 1) & 63;
											firmware_response.request = FIRMWARE_REQUEST_READ_PARAMS;
											memcpyram2ram(&firmware_response.params.operational_parameters, &params, sizeof(params));
											firmware_response_pending = true;
											break;

										case FIRMWARE_REQUEST_SET_PARAMS:
											if ((rxpacket->buf[6] & 0x40) && rxpacket->len == 12) {
#warning sanity checking
												memcpyram2ram(&params, rxpacket->buf + 8, sizeof(params));
												firmware_response.micropacket_length = sizeof(firmware_response) - sizeof(firmware_response.params);
												firmware_response.pipe = PIPE_FIRMWARE_IN;
												firmware_response.sequence = sequence[PIPE_FIRMWARE_IN];
												sequence[PIPE_FIRMWARE_IN] = (sequence[PIPE_FIRMWARE_IN] + 1) & 63;
												firmware_response.request = FIRMWARE_REQUEST_SET_PARAMS;
												firmware_response_pending = true;
											}
											break;

										case FIRMWARE_REQUEST_ROLLBACK_PARAMS:
#warning implement
										case FIRMWARE_REQUEST_COMMIT_PARAMS:
											params_commit(true);
											firmware_response.micropacket_length = sizeof(firmware_response) - sizeof(firmware_response.params);
											firmware_response.pipe = PIPE_FIRMWARE_IN;
											firmware_response.sequence = sequence[PIPE_FIRMWARE_IN];
											sequence[PIPE_FIRMWARE_IN] = (sequence[PIPE_FIRMWARE_IN] + 1) & 63;
											firmware_response.request = FIRMWARE_REQUEST_COMMIT_PARAMS;
											firmware_response_pending = true;
											break;

										case FIRMWARE_REQUEST_REBOOT:
											reboot_pending = REBOOT_PENDING_NEXT_PACKET;
											firmware_response.micropacket_length = sizeof(firmware_response) - sizeof(firmware_response.params);
											firmware_response.pipe = PIPE_FIRMWARE_IN;
											firmware_response.sequence = sequence[PIPE_FIRMWARE_IN];
											sequence[PIPE_FIRMWARE_IN] = (sequence[PIPE_FIRMWARE_IN] + 1) & 63;
											firmware_response.request = FIRMWARE_REQUEST_REBOOT;
											firmware_response_pending = true;
											break;

										default:
											/* Unknown firmware request.
											 * Send an error message. */
											error_reporting_add(FAULT_FIRMWARE_BAD_REQUEST);
											break;
									}
								} else {
									/* This is a continuation of an earlier bulk message. */
									if (flash_page_program_active) {
										/* A page program is in progress.
										 * Continue it with the data in this packet. */
										uint8_t i;
										__data const uint8_t *p;

										for (i = rxpacket->len - 7, p = rxpacket->buf + 7; i; --i, ++p) {
											spi_send(*p);
										}

										if (rxpacket->buf[6] & 0x40) {
											/* This packet is the end of the message.
											 * Terminate the write. */
											LAT_FLASH_CS = 1;

											/* Wait until complete. */
											LAT_FLASH_CS = 0;
											spi_send(0x05);
											while (spi_receive() & 0x01);
											LAT_FLASH_CS = 1;
										}
									}
								}
							}
						}
					} else {
						/* It's addressed to an unknown pipe. */
						error_reporting_add(FAULT_OUT_MICROPACKET_NOPIPE);
					}
				}
			} else if (rxpacket->xbee == 1 && rxpacket->len == 3 && rxpacket->buf[0] == XBEE_API_ID_TX_STATUS && rxpacket->buf[1] == txheader.frame_id && inbound_state == INBOUND_STATE_AWAITING_STATUS) {
				/* It's a transmit status packet from XBee #1 indicating the result of a transmission. */
				if (rxpacket->buf[2] == 0) {
					/* Transmission was successful.
					 * Clean up the IOVs and mark state. */
					inbound_state = INBOUND_STATE_IDLE;
					if (reboot_pending == REBOOT_PENDING_NEXT_PACKET) {
						/* If a reboot is scheduled for the next packet, advance the counter. */
						reboot_pending = REBOOT_PENDING_THIS_PACKET;
					} else if (reboot_pending == REBOOT_PENDING_THIS_PACKET) {
						/* If we were supposed to reboot, do that now. */
						INTCON = 0;
						Reset();
					}
				} else {
					/* Tranmission failed.
					 * Leave the packet in place and wait for the next polling time. */
					inbound_state = INBOUND_STATE_AWAITING_POLL;
				}
			}

			xbee_rxpacket_free(rxpacket);
		}

		if (INTCONbits.TMR0IF && inbound_state == INBOUND_STATE_AWAITING_STATUS) {
			/* Timeout waiting for transmit status, assume the serial line corrupted our data. */
			error_reporting_add(FAULT_XBEE1_TIMEOUT);
			inbound_state = INBOUND_STATE_AWAITING_POLL;
		}

		if (flash_burn_active) {
			uint8_t status;

			/* A flash burn operation (erase or program) was executing.
			 * Check if it's finished yet. */
			LAT_FLASH_CS = 0;
			spi_send(0x05);
			status = spi_receive();
			LAT_FLASH_CS = 1;

			if (!(status & 0x01)) {
				/* The operation is finished. */
				flash_burn_active = false;

				if (flash_reburn_params) {
					/* Reburn the parameters block. */
					params.flash_contents = FLASH_CONTENTS_NONE;
					params_commit(false);
					flash_reburn_params = false;
				}

				/* The response message has already been assembled.
				 * Queue it for transmission. */
				firmware_response_pending = true;
			}
		}

		if (!ADCON0bits.GO) {
			/* An analogue conversion has finished.
			 * Figure out which channel it was on. */
			if (!ADCON0bits.CHS1) {
				if (!ADCON0bits.CHS0) {
					/* Channel 0 -> battery voltage.
					 * Record result. */
					feedback_block.battery_voltage_raw = (ADRESH << 8) | ADRESL;
					parbus_txpacket.battery_voltage = (ADRESH << 8) | ADRESL;
					/* Start a conversion on channel 1. */
					ADCON0bits.CHS0 = 1;
					ADCON0bits.GO = 1;
				} else {
					/* Channel 1 -> thermistor.
					 * Record result. */
					feedback_block.dribbler_temperature_raw = (ADRESH << 8) | ADRESL;
					/* Start a conversion on channel 2. */
					ADCON0bits.CHS0 = 0;
					ADCON0bits.CHS1 = 1;
					ADCON0bits.GO = 1;
				}
			} else {
				if (!ADCON0bits.CHS0) {
					/* Channel 2 -> break beam.
					 * Start a conversion on channel 3. */
					ADCON0bits.CHS0 = 1;
					ADCON0bits.GO = 1;
				} else {
					/* Channel 3 -> miscellaneous device connector.
					 * Start a conversion on channel 0. */
					ADCON0bits.CHS0 = 0;
					ADCON0bits.CHS1 = 0;
					ADCON0bits.GO = 1;
				}
			}
		}

		if (PIR1bits.CCP1IF) {
			/* It's time to run a control loop iteration.
			 * Begin by reading back feedback from the FPGA. */
			{
				__data uint8_t *ptr = (__data uint8_t *) &parbus_rxpacket;
				uint8_t len = sizeof(parbus_rxpacket);
				(void) PMDIN1L;
				while (len--) {
					*ptr++ = PMDIN1L;
				}
			}

			/* Check out the results. */
			if (!parbus_rxpacket.flags.chicker_present) {
				error_reporting_add(FAULT_CHICKER_NOT_PRESENT);
			}

			if (drive_timeout && drive_block.flags.enable_robot) {
				uint8_t i;

				/* Count this iteration against the timeout. */
				--drive_timeout;

				/* Run the control loops. */
#warning this is not an actual control algorithm

				/* Prepare new orders for the FPGA. */
				parbus_txpacket.flags.enable_motors = 1;
				for (i = 0; i != 4; ++i) {
					if (drive_block.wheels[i] >= 0) {
						parbus_txpacket.flags.motors_direction &= ~(1 << i);
						parbus_txpacket.motors_power[i] = drive_block.wheels[i];
					} else {
						parbus_txpacket.flags.motors_direction |= 1 << i;
						parbus_txpacket.motors_power[i] = -drive_block.wheels[i];
					}
				}
			} else {
				/* Prepare scramming orders for the FPGA. */
				parbus_txpacket.flags.enable_motors = 0;
			}

			/* Prepare new orders. */
			parbus_txpacket.flags.enable_charger = drive_block.flags.charge;
			if (drive_block.flags.dribble) {
				parbus_txpacket.motors_power[4] = 25;
			} else {
				parbus_txpacket.motors_power[4] = 0;
			}
			if (kick_power) {
				parbus_txpacket.kick_power = kick_power;
				parbus_txpacket.flags.kick_sequence = !parbus_txpacket.flags.kick_sequence;
				kick_power = 0;
			}

			/* Fill the radio feedback block. */
			feedback_block.flags.valid = parbus_rxpacket.flags.feedback_ok;
			feedback_block.flags.capacitor_charged = parbus_rxpacket.capacitor_voltage > ((uint16_t) (215.0 / (220000.0 + 2200.0) * 2200.0 / 3.3 * 4095.0)) && !kick_power;
			feedback_block.capacitor_voltage_raw = parbus_rxpacket.capacitor_voltage;

			/* Send orders to the FPGA. */
			{
				__data const uint8_t *ptr = (__data uint8_t *) &parbus_txpacket;
				uint8_t len = sizeof(parbus_txpacket);
				while (len--) {
					PMDIN1L = *ptr++;
				}
			}

			/* Clear interrupt flag. */
			PIR1bits.CCP1IF = 0;
		}
	}
}

