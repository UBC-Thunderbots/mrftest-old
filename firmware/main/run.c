#include "run.h"
#include "crc.h"
#include "drive.h"
#include "error_reporting.h"
#include "feedback.h"
#include "fw.h"
#include "kick_packet.h"
#include "params.h"
#include "parbus.h"
#include "pins.h"
#include "pipes.h"
#include "spi.h"
#include "test_mode_packet.h"
#include "wheel_controller.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <delay.h>
#include <pic18fregs.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * \brief Whether to run a scripted experiment.
 */
#define EXPERIMENT_MODE 0

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
 * \brief The number of 5ms ticks to make up the autokick timeout.
 */
#define AUTOKICK_LOCKOUT_TIME 50

/**
 * \brief The minimum delay, in 5ms ticks, between changing the hard motor enable flag and the soft motor enable flag.
 *
 * Motor powerup is staged via two separate controls.
 * The “hard” enable is a line from a physical pin on the microcontroller.
 * The “soft” enable is a pair of bit flags sent over the parallel bus to the FPGA, one for the wheels and one for the dribbler.
 *
 * When the hard enable flag is cleared, all motor phases are forced high, ignoring the FPGA completely.
 * When the hard enable flag is set, motor phases receive their polarities from the FPGA.
 *
 * When the soft enable flag for a motor is cleared, the FPGA attempts to float all motor phases.
 * When the soft enable flag for a motor is set, the FPGA outputs commutated PWM at the most-recently-specified duty cycle to the motor phases.
 *
 * Because the hard enable flag is not wired to the FPGA, the dead-band generator is unaware of its value.
 * Therefore, if both hard and soft enable flags are clear, the FPGA's dead-band generator will believe the motor phases are floating, when they are actually held high.
 * If the hard and soft flags were to be enabled simultaneously, shootthrough would occur on the phase transitioning to low polarity, because the dead-band generator would not delay the transition, believing the phase to have been floating previously.
 * If the hard and soft flags were to be disabled simultaneously, shootthrough would occur on the phase transitioning from a low polarity, because the hard enable flag being cleared forces that phase (along with all others) to instantly go high.
 *
 * To avoid shootthrough in the enabling case, the soft enable flag's rising edge trails the hard enable flag's rising edge, forcing the phases to float for the time difference before driving to their final values when the soft flag rises.
 * To avoid shootthrough in the disabling case, the hard enable flag's falling edge trails the soft enable flag's falling edge, forcing the phases to float for the time difference before all going high.
 *
 * This is the number of ticks of time difference that must be applied in each case.
 */
#define MOTOR_HARD_DISABLE_TICK_COUNT 20

/**
 * \brief The capacitor voltage above which the "charged" LED should light.
 */
#define CAPACITOR_LED_THRESHOLD ((unsigned int) (30.0 / (220000.0 + 2200.0) * 2200.0 / 3.3 * 4096.0))

/**
 * \brief The capacitor voltage above which the safe discharge pulse generator should fire if the charger is disabled.
 */
#define CAPACITOR_SAFE_DISCHARGE_THRESHOLD ((unsigned int) (25.0 / (220000.0 + 2200.0) * 2200.0 / 3.3 * 4096.0))

/**
 * \brief The width, in microseconds, of the pulse to use to safely discharge the capacitors.
 */
#define CAPACITOR_SAFE_DISCHARGE_PULSE_WIDTH 100

/**
 * \brief The voltage level to send to the dribbler motor when the ball is not in the beam.
 */
#define DRIBBLE_POWER_NO_BALL 180

/**
 * \brief The voltage level to send to the dribbler motor when the ball is in the beam.
 */
#define DRIBBLE_POWER_WITH_BALL 80

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

static void run_wheel(uint16_t index, int16_t feedback, int16_t setpoint, wheel_controller_ctx_t *ctx, BOOL enable_controllers) {
	int16_t power;
	uint16_t encoded;

	/* Read the feedback value and run the control loop. */
	if (enable_controllers) {
		power = wheel_controller_iter(setpoint, feedback, ctx);
	} else {
		wheel_controller_clear(ctx);
		power = setpoint;
	}

	/* Encode the power level into 9-bit sign-magnitude form. */
	encoded = 0;
	if (power < 0) {
		power = -power;
		encoded = 0x100;
	}
	if (power > 255) {
		power = 255;
	}
	encoded |= power;

	/* Send the new power level. */
	parbus_write(1 + (index - 1), encoded);
}

void run(void) {
	__data xbee_rxpacket_t *rxpacket;
	__data const uint8_t *rxptr;
	static xbee_tx16_header_t txheader = { XBEE_API_ID_TX_16, 0x00, 0x7B, 0x41, 0x00 };
	static xbee_txpacket_iovec_t txiovs[5];
	static xbee_txpacket_t txpkt;
	inbound_state_t inbound_state = INBOUND_STATE_IDLE;
	static const uint8_t FEEDBACK_MICROPACKET_HEADER[2] = { 2 + sizeof(feedback_block_t), PIPE_FEEDBACK };
	static feedback_block_t txpkt_feedback_shadow;
	static uint8_t sequence[PIPE_MAX + 1];
#if EXPERIMENT_MODE
	static uint8_t experiment_data[256];
	static struct {
		uint8_t micropacket_length;
		uint8_t pipe;
		uint8_t sequence;
		uint8_t block_index;
	} experiment_header = { sizeof(experiment_header) + 8, PIPE_EXPERIMENT_DATA, 0, 0 };
	uint8_t index = 0;
	enum {
		EXPERIMENT_STATE_HALT,
		EXPERIMENT_STATE_PRE,
		EXPERIMENT_STATE_RUNNING,
		EXPERIMENT_STATE_DONE,
	} experiment_state = EXPERIMENT_STATE_HALT;
	uint8_t experiment_control_code = 0;
#else
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
	uint8_t status, i;
	uint16_t crc;
	uint8_t motor_hard_disable_tick_counter = MOTOR_HARD_DISABLE_TICK_COUNT;
	static struct {
		uint8_t micropacket_length;
		uint8_t pipe;
		uint8_t sequence;
	} autokick_indicator_micropacket = { sizeof(autokick_indicator_micropacket), PIPE_AUTOKICK_INDICATOR, 0 };
	BOOL autokick_indicator_micropacket_pending = false;
	static wheel_controller_ctx_t wheel_controller_ctxs[4];
#endif
	uint8_t drive_timeout = 0;
	const uint8_t robot_number = params.robot_number;
	BOOL fpga_ok;
	uint8_t autokick_lockout_time = 0;
	uint8_t battery_fail_count = 0;

	/* Clear state. */
	memset(sequence, 0, sizeof(sequence));
	txiovs[0].len = sizeof(txheader);
	txiovs[0].ptr = &txheader;
	txiovs[1].len = sizeof(FEEDBACK_MICROPACKET_HEADER);
	txiovs[1].ptr = FEEDBACK_MICROPACKET_HEADER;
	txiovs[2].len = sizeof(txpkt_feedback_shadow);
	txiovs[2].ptr = &txpkt_feedback_shadow;
	txpkt.iovs = txiovs;
	wheel_controller_clear(&wheel_controller_ctxs[0]);
	wheel_controller_clear(&wheel_controller_ctxs[1]);
	wheel_controller_clear(&wheel_controller_ctxs[2]);
	wheel_controller_clear(&wheel_controller_ctxs[3]);

	if (params.flash_contents == FLASH_CONTENTS_FPGA) {
		/* Read the magic signature from the FPGA over the parallel bus. */
		if (parbus_read(0) == 0x468D) {
			LAT_LED4 = 1;
			fpga_ok = true;
		} else {
			error_reporting_add(FAULT_FPGA_COMM_ERROR);
			fpga_ok = false;
			LAT_FPGA_PROG_B = 0;
		}
	} else {
		fpga_ok = false;
		error_reporting_add(FAULT_FPGA_NO_BITSTREAM);
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
		/* Kick the watchdog. */
		ClrWdt();

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
			if (rxpacket->xbee == 0 && rxpacket->len > 5 && rxpacket->buf[0] == XBEE_API_ID_RX_16 && rxpacket->buf[1] == 0x7B && rxpacket->buf[2] == 0x40) {
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
								if (robot == robot_number) {
									/* It's addressed to us. */
									if (pipe == PIPE_DRIVE) {
										if (rxptr[0] == 2 + DRIVE_PACKET_BYTES) {
											/* It's new drive data. */
											drive_packet_decode(&drive_block, rxptr + 2);
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

					if (rxpacket->buf[5] == robot_number && (inbound_state == INBOUND_STATE_IDLE || inbound_state == INBOUND_STATE_AWAITING_POLL)) {
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
#if EXPERIMENT_MODE
							if (experiment_state == EXPERIMENT_STATE_DONE) {
								/* Send some of the experiment data. */
								experiment_header.sequence = sequence[PIPE_EXPERIMENT_DATA];
								sequence[PIPE_EXPERIMENT_DATA] = (sequence[PIPE_EXPERIMENT_DATA] + 1) & 63;
								experiment_header.block_index = index;
								txiovs[txpkt.num_iovs].len = sizeof(experiment_header);
								txiovs[txpkt.num_iovs].ptr = &experiment_header;
								++txpkt.num_iovs;
								txiovs[txpkt.num_iovs].len = 8;
								txiovs[txpkt.num_iovs].ptr = experiment_data + index;
								++txpkt.num_iovs;

								/* Advance the counter. */
								index += 8;
								if (!index) {
									experiment_state = EXPERIMENT_STATE_HALT;
								}
							}
#else
							if (firmware_response_pending) {
								/* A firmware response is pending.
								 * Add it to the packet. */
								txiovs[txpkt.num_iovs].len = firmware_response.micropacket_length;
								txiovs[txpkt.num_iovs].ptr = &firmware_response;
								++txpkt.num_iovs;
								firmware_response_pending = false;
							}
							if (autokick_indicator_micropacket_pending) {
								/* An autokick indicator micropacket is pending.
								 * Add it to the packet. */
								txiovs[txpkt.num_iovs].len = autokick_indicator_micropacket.micropacket_length;
								txiovs[txpkt.num_iovs].ptr = &autokick_indicator_micropacket;
								++txpkt.num_iovs;
								autokick_indicator_micropacket_pending = false;
							}
#endif
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
					/* It's a packet containing a single message message. */
					if (rxpacket->buf[5] <= PIPE_MAX && ((1 << rxpacket->buf[5]) & PIPE_OUT_MASK & PIPE_MESSAGE_MASK)) {
						/* It's addressed to an existing message pipe in the right direction. */
						if ((rxpacket->buf[6] & 63) == sequence[rxpacket->buf[5]]) {
							/* The sequence number is correct. */
							sequence[rxpacket->buf[5]] = (sequence[rxpacket->buf[5]] + 1) & 63;
							if (rxpacket->buf[5] == PIPE_KICK) {
								/* The packet contains a kick request. */
								if (rxpacket->len == 7 + KICK_PACKET_BYTES) {
									kick_packet_t kick_pkt;
									kick_packet_decode(&kick_pkt, rxpacket->buf + 7);
									parbus_write(10, kick_pkt.width1 * 32);
									parbus_write(11, kick_pkt.width2 * 32);
									parbus_write(12, (kick_pkt.offset * 32) | (kick_pkt.offset_sign ? 0x8000 : 0x0000));
									autokick_lockout_time = AUTOKICK_LOCKOUT_TIME;
								} else {
									error_reporting_add(FAULT_OUT_MICROPACKET_BAD_LENGTH);
								}
							} else if (rxpacket->buf[5] == PIPE_TEST_MODE) {
								/* The packet contains a test mode. */
								if (rxpacket->len == 7 + TEST_MODE_PACKET_BYTES) {
									/* Enable whatever test mode was requested. */
									test_mode_packet_t test_mode_pkt;
									test_mode_packet_decode(&test_mode_pkt, rxpacket->buf + 7);
									parbus_write(6, (test_mode_pkt.test_class << 8) | test_mode_pkt.test_index);
								} else {
									error_reporting_add(FAULT_OUT_MICROPACKET_BAD_LENGTH);
								}
#if EXPERIMENT_MODE
							} else if (rxpacket->buf[5] == PIPE_EXPERIMENT_CONTROL) {
								if (rxpacket->len == 7 + 1) {
									/* Start an experiment. */
									experiment_control_code = rxpacket->buf[7];
									experiment_state = EXPERIMENT_STATE_PRE;
								} else {
									error_reporting_add(FAULT_OUT_MICROPACKET_BAD_LENGTH);
								}
#else
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
										if (rxpacket->len == 13) {
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
										firmware_response.params.build_signatures.flash_crc = parbus_read(7);
										firmware_response_pending = true;
										break;

									default:
										/* Unknown firmware request.
										 * Send an error message. */
										error_reporting_add(FAULT_FIRMWARE_BAD_REQUEST);
										break;
								}
#endif
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
#if !EXPERIMENT_MODE
					if (reboot_pending == REBOOT_PENDING_NEXT_PACKET) {
						/* If a reboot is scheduled for the next packet, advance the counter. */
						reboot_pending = REBOOT_PENDING_THIS_PACKET;
					} else if (reboot_pending == REBOOT_PENDING_THIS_PACKET) {
						/* If we were supposed to reboot, do that now. */
						INTCON = 0;
						Reset();
					}
#endif
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

#if !EXPERIMENT_MODE
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

				/* Change the parameters block to reflect no data. */
				params_load();
				params.flash_contents = FLASH_CONTENTS_NONE;
				params_commit();
			}
		}
#endif

		if (!ADCON0bits.GO) {
			/* An analogue conversion has finished.
			 * Figure out which channel it was on. */
			if (!ADCON0bits.CHS1) {
				if (!ADCON0bits.CHS0) {
					/* Channel 0 -> battery voltage.
					 * Record result. */
					feedback_block.battery_voltage_raw = (ADRESH << 8) | ADRESL;
					/* Check for cutoff level. */
					if (feedback_block.battery_voltage_raw < (uint16_t) (BATTERY_VOLTAGE_MIN / (BATTERY_VOLTAGE_R_TOP + BATTERY_VOLTAGE_R_BOTTOM) * BATTERY_VOLTAGE_R_BOTTOM / 3.3 * 1024.0)) {
						if (!++battery_fail_count) {
							parbus_write(0, 0);
							parbus_write(1, 0);
							parbus_write(2, 0);
							parbus_write(3, 0);
							parbus_write(4, 0);
							parbus_write(5, 0);
							parbus_write(6, 0);
							delay1mtcy(1);
							LAT_MOTOR_ENABLE = 0;
							LAT_DRIB_BEAM_OUT = 0;
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
					} else {
						battery_fail_count = 0;
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
					feedback_block.flags.ball_in_beam = feedback_block.break_beam_raw >= 512;
					if (feedback_block.flags.ball_in_beam) {
						LAT_LED1 = 0;
					} else {
						LAT_LED1 = 1;
					}
					/* Start a conversion on channel 3. */
					ADCON0bits.CHS0 = 1;
					ADCON0bits.GO = 1;
					/* Fire autokick if ready. */
					if (drive_block.enable_autokick && feedback_block.flags.ball_in_beam && !autokick_lockout_time && feedback_block.flags.capacitor_charged) {
						parbus_write(10, drive_block.autokick_width1 * 32);
						parbus_write(11, drive_block.autokick_width2 * 32);
						parbus_write(12, (drive_block.autokick_offset * 32) | (drive_block.autokick_offset_sign ? 0x8000 : 0x0000));
						autokick_lockout_time = AUTOKICK_LOCKOUT_TIME;
#if !EXPERIMENT_MODE
						if (!autokick_indicator_micropacket_pending) {
							autokick_indicator_micropacket.sequence = sequence[PIPE_AUTOKICK_INDICATOR];
							sequence[PIPE_AUTOKICK_INDICATOR] = (sequence[PIPE_AUTOKICK_INDICATOR] + 1) & 63;
							autokick_indicator_micropacket_pending = true;
						}
#endif
					}
				} else {
					/* Channel 3 -> miscellaneous device connector.
					 * Start a conversion on channel 0. */
					ADCON0bits.CHS0 = 0;
					ADCON0bits.CHS1 = 0;
					ADCON0bits.GO = 1;

					/* Now that all ADC channels have finished converting at least once, mark the feedback packet valid. */
					feedback_block.flags.valid = 1;
				}
			}
		}

		if (PIR1bits.CCP1IF && fpga_ok) {
			uint8_t flags_in, flags_out = 0;
			BOOL wheels_controlled = false;
			int16_t encoder_readings[4];

			/* Auto-kick timeout should expire eventually. */
			if (autokick_lockout_time && !feedback_block.flags.ball_in_beam) {
				--autokick_lockout_time;
			}

			/* Read the general flags. */
			flags_in = parbus_read(1);

			/* It's time to run a control loop iteration.
			 * Latch the encoder counts. */
			parbus_write(9, 0);

			/* Read the optical encoders. */
			encoder_readings[0] = parbus_read(2);
			encoder_readings[1] = parbus_read(3);
			encoder_readings[2] = parbus_read(4);
			encoder_readings[3] = parbus_read(5);

			/* Check if there's a chicker present; if not, we should report an error. */
			if (!(flags_in & 0x02)) {
				error_reporting_add(FAULT_CHICKER_NOT_PRESENT);
			}

#if EXPERIMENT_MODE
			switch (experiment_state) {
				case EXPERIMENT_STATE_HALT:
					/* Turn off the motors. */
					parbus_write(0, 0);
					parbus_write(1, 0);
					parbus_write(2, 0);
					parbus_write(3, 0);
					parbus_write(4, 0);
					LAT_MOTOR_ENABLE = 0;
					break;

				case EXPERIMENT_STATE_PRE:
					/* Run motor HIGH_NYBBLE(CONTROL_CODE) at power 25. */
					parbus_write(0, 0x01);
					parbus_write(1 + (experiment_control_code >> 4), 25);
					LAT_MOTOR_ENABLE = 1;

					/* Tick. */
					if (!++index) {
						experiment_state = EXPERIMENT_STATE_RUNNING;
					}
					break;

				case EXPERIMENT_STATE_RUNNING:
					/* Run motor HIGH_NYBBLE(CONTROL_CODE) at power 75. */
					parbus_write(0, 0x01);
					parbus_write(1 + (experiment_control_code >> 4), 75);
					LAT_MOTOR_ENABLE = 1;

					/* Record data. */
					experiment_data[index] = (uint8_t) parbus_read(2 + (experiment_control_code & 0x0F));

					/* Tick. */
					if (!++index) {
						experiment_state = EXPERIMENT_STATE_DONE;
					}
					break;

				case EXPERIMENT_STATE_DONE:
					/* Turn off the motors. */
					parbus_write(0, 0);
					LAT_MOTOR_ENABLE = 0;
					break;
			}
#else
			/* Check for timeout. */
			if (drive_timeout) {
				/* No timeout yet. */
				--drive_timeout;
			} else {
				/* Wheels, dribbler, charger, and auto-kick should turn off on timeout. */
				drive_block.enable_wheels = false;
				drive_block.enable_charger = false;
				drive_block.enable_dribbler = false;
				drive_block.enable_autokick = false;
			}

			/* Check if any of the motors are enabled (wheels or dribbler). */
			if (drive_block.enable_wheels || drive_block.enable_dribbler) {
				/* Wheels or dribbler enabled. */
				LAT_MOTOR_ENABLE = 1;

				/* Check for motor hard-disable timeout. */
				if (motor_hard_disable_tick_counter == 0) {
					/* Enough time has passed since hard-enable. We may now soft-enable as well. */

					/* Control the wheels if and only if so ordered. */
					if (drive_block.enable_wheels) {
						/* Run the control loops. */
						run_wheel(1, encoder_readings[0], drive_block.wheel1, &wheel_controller_ctxs[0], drive_block.enable_controllers);
						run_wheel(2, encoder_readings[1], drive_block.wheel2, &wheel_controller_ctxs[1], drive_block.enable_controllers);
						run_wheel(3, encoder_readings[2], drive_block.wheel3, &wheel_controller_ctxs[2], drive_block.enable_controllers);
						run_wheel(4, encoder_readings[3], drive_block.wheel4, &wheel_controller_ctxs[3], drive_block.enable_controllers);
						wheels_controlled = true;

						/* Order the motors enabled. */
						flags_out |= 0x01;
					}

					/* Enable the dribbler if and only if so ordered and temperature is acceptable. */
					if (drive_block.enable_dribbler && feedback_block.dribbler_temperature_raw >= 200 && feedback_block.dribbler_temperature_raw <= 499) {
						flags_out |= 0x04;
						parbus_write(5, feedback_block.flags.ball_in_beam ? DRIBBLE_POWER_WITH_BALL : DRIBBLE_POWER_NO_BALL);
					} else {
						parbus_write(5, 0);
					}
				} else {
					/* The hard-enable only happened very recently. We should wait a bit longer. */
					--motor_hard_disable_tick_counter;
				}
			} else {
				/* Wheels and dribbler both disabled. */

				/* Clear dribbler power setting. */
				parbus_write(5, 0);

				/* Check for motor hard-disable timeout. */
				if (motor_hard_disable_tick_counter == MOTOR_HARD_DISABLE_TICK_COUNT) {
					/* Enough time has passed since soft-disable. We may now hard-disable as well. */
					LAT_MOTOR_ENABLE = 0;
				} else {
					/* The soft-disable only happened very recently. We should wait a bit longer. */
					++motor_hard_disable_tick_counter;
					LAT_MOTOR_ENABLE = 1;
				}
			}

			/* Receive current capacitor voltage. */
			feedback_block.capacitor_voltage_raw = parbus_read(6);

			/* If voltage is high, light an LED. */
			if (feedback_block.capacitor_voltage_raw > CAPACITOR_LED_THRESHOLD) {
				LAT_LED2 = 1;
			} else {
				LAT_LED2 = 0;
			}

			if (drive_block.enable_charger) {
				/* Host requests charger to be enabled. */
				flags_out |= 0x02;
			} else {
				/* Host requests charger to be disabled. */
				if (feedback_block.capacitor_voltage_raw > CAPACITOR_SAFE_DISCHARGE_THRESHOLD) {
					/* Capacitor voltage is high enough we should issue a tiny pulse to discharge it. */
					parbus_write(10, CAPACITOR_SAFE_DISCHARGE_PULSE_WIDTH);
					parbus_write(11, CAPACITOR_SAFE_DISCHARGE_PULSE_WIDTH);
					parbus_write(12, 0);
				}
			}

			/* Send the general operational flags. */
			parbus_write(0, flags_out);

			/* Fill the radio feedback block. */
			if (flags_in & 0x04) {
				feedback_block.flags.capacitor_charged = true;
			} else {
				feedback_block.flags.capacitor_charged = false;
			}
			if (flags_in & 0x08) {
				feedback_block.flags.hall_stuck = true;
			}
			if (flags_in & 0x10) {
				feedback_block.flags.encoder_1_stuck = true;
			} else {
				feedback_block.flags.encoder_1_stuck = false;
			}
			if (flags_in & 0x20) {
				feedback_block.flags.encoder_2_stuck = true;
			} else {
				feedback_block.flags.encoder_2_stuck = false;
			}
			if (flags_in & 0x40) {
				feedback_block.flags.encoder_3_stuck = true;
			} else {
				feedback_block.flags.encoder_3_stuck = false;
			}
			if (flags_in & 0x80) {
				feedback_block.flags.encoder_4_stuck = true;
			} else {
				feedback_block.flags.encoder_4_stuck = false;
			}

			/* Clear controllers if not used this tick. */
			if (!wheels_controlled) {
				wheel_controller_clear(&wheel_controller_ctxs[0]);
				wheel_controller_clear(&wheel_controller_ctxs[1]);
				wheel_controller_clear(&wheel_controller_ctxs[2]);
				wheel_controller_clear(&wheel_controller_ctxs[3]);
			}
#endif

			/* Clear interrupt flag. */
			PIR1bits.CCP1IF = 0;
		}
	}
}

