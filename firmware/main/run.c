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
#include "wheel_controller.h"
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
 * \brief The minimum voltage the battery can reach before the system locks out.
 */
#define BATTERY_VOLTAGE_MIN 13.0

/**
 * \brief The resistor on the top half of the divider for measuring battery voltage.
 */
#define BATTERY_VOLTAGE_R_TOP 1500.0

/**
 * \brief The resistor on the bottom half of the divider for measuring battery voltage.
 */
#define BATTERY_VOLTAGE_R_BOTTOM 330.0

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
	static xbee_txpacket_iovec_t txiovs[4];
	static xbee_txpacket_t txpkt;
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
	BOOL flash_erase_active = false;
	static params_t flash_temp_params;
	static uint8_t page_buffer[256];
	uint8_t drive_timeout = 0;
	const uint8_t robot_number = params.robot_number;
	uint8_t status, i;
	uint16_t crc;
	BOOL fpga_ok;

	/* Clear state. */
	memset(sequence, 0, sizeof(sequence));
	txiovs[0].len = sizeof(txheader);
	txiovs[0].ptr = &txheader;
	txiovs[1].len = sizeof(FEEDBACK_MICROPACKET_HEADER);
	txiovs[1].ptr = FEEDBACK_MICROPACKET_HEADER;
	txiovs[2].len = sizeof(txpkt_feedback_shadow);
	txiovs[2].ptr = &txpkt_feedback_shadow;
	txpkt.iovs = txiovs;

	/* Read the magic signature from the FPGA over the parallel bus. */
	if (parbus_read(0) == 0x468D) {
		LAT_MOTOR_ENABLE = 1;
		fpga_ok = true;
	} else {
		error_reporting_add(FAULT_FPGA_COMM_ERROR);
		fpga_ok = false;
		LAT_FPGA_PROG_B = 0;
	}

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

	/* Turn on the laser. */
	LAT_DRIB_BEAM_OUT = 1;

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
		if (xbee_txpacket_dequeue() == &txpkt) {
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
							/* We should resend the last packet. */
							/* We do, however, want a new frame number. */
							if (!++txheader.frame_id) {
								txheader.frame_id = 1;
							}
							xbee_txpacket_queue(&txpkt, 1);
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
							/* Send the packet. */
							xbee_txpacket_queue(&txpkt, 1);
						}

						/* Mark state. */
						inbound_state = INBOUND_STATE_SENDING;
					}
				} else if (rxpacket->len == 6 && rxpacket->buf[5] == 0xFF) {
					/* It's a discovery and synchronization packet.
					 * Clear our sequence numbers. */
					memset(sequence, 0, sizeof(sequence));
				} else {
					/* It's a packet containing a single interrupt message. */
					if (rxpacket->buf[5] <= PIPE_MAX && ((1 << rxpacket->buf[5]) & PIPE_OUT_MASK & PIPE_INTERRUPT_MASK)) {
						/* It's addressed to an existing interrupt pipe in the right direction. */
						if ((rxpacket->buf[6] & 63) == sequence[rxpacket->buf[5]]) {
							/* The sequence number is correct. */
							sequence[rxpacket->buf[5]] = (sequence[rxpacket->buf[5]] + 1) & 63;
							if (rxpacket->buf[5] == PIPE_FAULT_OUT) {
								/* The packet contains a fault clearing request. */
#warning implement
							} else if (rxpacket->buf[5] == PIPE_KICK) {
								/* The packet contains a kick request. */
								if (rxpacket->len == 14) {
									parbus_write(10, rxpacket->buf[8] | (rxpacket->buf[9] << 8));
									parbus_write(11, rxpacket->buf[10] | (rxpacket->buf[11] << 8));
									parbus_write(12, rxpacket->buf[12] | (rxpacket->buf[13] << 8));
								} else {
									error_reporting_add(FAULT_OUT_MICROPACKET_BAD_LENGTH);
								}
							} else if (rxpacket->buf[5] == PIPE_FIRMWARE_OUT) {
								/* The packet contains a firmware request. */
								firmware_response_pending = false;
								switch (rxpacket->buf[7]) {
									case FIRMWARE_REQUEST_CHIP_ERASE:
										/* Suspend inbound communication so data will not be lost. */
										xbee_rxpacket_suspend();

										/* Do not take interrupts during this time. */
										INTCONbits.GIEH = 0;

										/* Set write enable latch. */
										LAT_FLASH_CS = 0;
										spi_send(0x06);
										LAT_FLASH_CS = 1;

										/* Send the erase instruction. */
										LAT_FLASH_CS = 0;
										spi_send(0xC7);
										LAT_FLASH_CS = 1;

										/* Now accept interrupts. */
										INTCONbits.GIEH = 1;

										/* Resume inbound communication. */
										xbee_rxpacket_resume();

										/* Prepare a response, but do not queue it yet (we will do that when the operation finishes). */
										firmware_response.micropacket_length = sizeof(firmware_response) - sizeof(firmware_response.params);
										firmware_response.pipe = PIPE_FIRMWARE_IN;
										firmware_response.sequence = sequence[PIPE_FIRMWARE_IN];
										sequence[PIPE_FIRMWARE_IN] = (sequence[PIPE_FIRMWARE_IN] + 1) & 63;
										firmware_response.request = FIRMWARE_REQUEST_CHIP_ERASE;

										/* Remember that the operation is ongoing. */
										flash_erase_active = true;
										break;

									case FIRMWARE_REQUEST_FILL_PAGE_BUFFER:
										if ((uint16_t) rxpacket->buf[8] + rxpacket->len - 9 <= 256) {
											/* Copy the payload into the page buffer. */
											memcpyram2ram(page_buffer + rxpacket->buf[8], rxpacket->buf + 9, rxpacket->len - 9);
										}
										break;

									case FIRMWARE_REQUEST_PAGE_PROGRAM:
										if (rxpacket->len == 12) {
											/* CRC-check the page buffer. */
											i = 0;
											rxptr = page_buffer;
											crc = CRC16_EMPTY;
											do {
												crc = crc_update(crc, *rxptr++);
											} while (--i);
											if (crc == (rxpacket->buf[10] | (rxpacket->buf[11] << 8))) {
												/* Suspend inbound communication so data will not be lost. */
												xbee_rxpacket_suspend();

												/* Do not take interrupts during this time. */
												INTCONbits.GIEH = 0;

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
												rxptr = page_buffer;
												do {
													spi_send(*rxptr++);
												} while (--i);
												LAT_FLASH_CS = 1;

												/* Wait until complete. */
												LAT_FLASH_CS = 0;
												spi_send(0x05);
												while (spi_receive() & 0x01);
												LAT_FLASH_CS = 1;

												/* Now accept interrupts. */
												INTCONbits.GIEH = 1;

												/* Resume inbound communication. */
												xbee_rxpacket_resume();

												/* Queue a response. */
												firmware_response.micropacket_length = sizeof(firmware_response) - sizeof(firmware_response.params);
												firmware_response.pipe = PIPE_FIRMWARE_IN;
												firmware_response.sequence = sequence[PIPE_FIRMWARE_IN];
												sequence[PIPE_FIRMWARE_IN] = (sequence[PIPE_FIRMWARE_IN] + 1) & 63;
												firmware_response.request = FIRMWARE_REQUEST_PAGE_PROGRAM;
												firmware_response_pending = true;
											}
										}
										break;

									case FIRMWARE_REQUEST_CRC_BLOCK:
										leds_show_number(8);
										if (rxpacket->len == 13) {
											uint16_t counter, crc = CRC16_EMPTY;

											/* Send the Read Data instruction and the address. */
											leds_show_number(9);
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
											leds_show_number(10);
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
										if (rxpacket->len == 14) {
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
										break;

									case FIRMWARE_REQUEST_COMMIT_PARAMS:
										params_commit();
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

									case FIRMWARE_REQUEST_READ_BUILD_SIGNATURES:
										firmware_response.micropacket_length = sizeof(firmware_response) - sizeof(firmware_response.params) + sizeof(firmware_response.params.build_signatures);
										firmware_response.pipe = PIPE_FIRMWARE_IN;
										firmware_response.sequence = sequence[PIPE_FIRMWARE_IN];
										sequence[PIPE_FIRMWARE_IN] = (sequence[PIPE_FIRMWARE_IN] + 1) & 63;
										firmware_response.request = FIRMWARE_REQUEST_READ_BUILD_SIGNATURES;
										firmware_response.params.build_signatures.firmware_crc = firmware_crc;
										firmware_response.params.build_signatures.flash_crc = flash_crc;
										firmware_response_pending = true;
										break;

									default:
										/* Unknown firmware request.
										 * Send an error message. */
										error_reporting_add(FAULT_FIRMWARE_BAD_REQUEST);
										break;
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

		if (flash_erase_active) {
			/* A flash erase operation was executing.
			 * Check if it's finished yet. */
			LAT_FLASH_CS = 0;
			spi_send(0x05);
			status = spi_receive();
			LAT_FLASH_CS = 1;

			if (!(status & 0x01)) {
				/* The operation is finished. */
				flash_erase_active = false;

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
					/* Check for cutoff level. */
					if (feedback_block.battery_voltage_raw < (uint16_t) (BATTERY_VOLTAGE_MIN / (BATTERY_VOLTAGE_R_TOP + BATTERY_VOLTAGE_R_BOTTOM) * BATTERY_VOLTAGE_R_BOTTOM / 3.3 * 1023.0)) {
						parbus_write(0, 0);
						parbus_write(1, 0);
						parbus_write(2, 0);
						parbus_write(3, 0);
						parbus_write(4, 0);
						parbus_write(5, 0);
						parbus_write(6, 0);
						delay1mtcy(1);
						LAT_MOTOR_ENABLE = 0;
						LAT_XBEE0_SLEEP = 1;
						LAT_XBEE1_SLEEP = 1;
						LAT_LED1 = 0;
						LAT_LED2 = 0;
						LAT_LED3 = 0;
						LAT_LED4 = 0;
						INTCONbits.GIEH = 0;
						for (;;) {
							Sleep();
						}
					}
					/* Send data to FPGA so it can scale boost converter timings. */
					parbus_write(8, feedback_block.battery_voltage_raw);
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
					 * Record result. */
					feedback_block.break_beam_raw = (ADRESH << 8) | ADRESL;
					feedback_block.flags.ball_in_beam = feedback_block.break_beam_raw >= 300;
					/* Start a conversion on channel 3. */
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

		if (PIR1bits.CCP1IF && fpga_ok) {
			uint16_t flags_out = 0;

			/* It's time to run a control loop iteration.
			 * Latch the encoder counts. */
			parbus_write(9, 0);

			/* Check if there's a chicker present; if not, we should report an error. */
			if (!(parbus_read(1) & 0x02)) {
				error_reporting_add(FAULT_CHICKER_NOT_PRESENT);
			}

			if (drive_timeout && drive_block.flags.enable_robot) {
				uint8_t i;

				/* Count this iteration against the timeout. */
				--drive_timeout;

				/* Run the control loops. */
				for (i = 0; i != 4; ++i) {
					int16_t power = wheel_controller_iter(drive_block.wheels[i], parbus_read(2 + i));
					uint16_t encoded = 0;
					if (power < 0) {
						power = -power;
						encoded = 0x100;
					}
					if (power > 255) {
						power = 255;
					}
					encoded |= power;
					parbus_write(1 + i, encoded);
				}

				/* Order the motors enabled. */
				flags_out |= 0x01;
			}

			/* Enable the charger if and only if so ordered. */
			if (drive_timeout && drive_block.flags.charge) {
				flags_out |= 0x02;
			}

			/* Enable the dribbler if and only if so ordered. */
			if (drive_block.flags.dribble && feedback_block.dribbler_temperature_raw >= 200 && feedback_block.dribbler_temperature_raw <= 499) {
				parbus_write(5, params.dribble_power);
			} else {
				parbus_write(5, 0);
			}

			/* Enable whatever test mode was requested. */
			parbus_write(6, drive_block.test_mode);

			/* Send the general operational flags. */
			parbus_write(0, flags_out);

			/* Fill the radio feedback block. */
			feedback_block.flags.valid = 1;
			feedback_block.capacitor_voltage_raw = parbus_read(6);
			feedback_block.flags.capacitor_charged = feedback_block.capacitor_voltage_raw > ((uint16_t) (215.0 / (220000.0 + 2200.0) * 2200.0 / 3.3 * 4095.0));

			/* Clear interrupt flag. */
			PIR1bits.CCP1IF = 0;
		}
	}
}

