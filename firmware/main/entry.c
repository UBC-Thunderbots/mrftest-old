#include "adc.h"
#include "chicker.h"
#include "control.h"
#include "debug.h"
#include "device_dna.h"
#include "dribbler.h"
#include "flash.h"
#include "globals.h"
#include "io.h"
#include "led.h"
#include "log.h"
#include "motor.h"
#include "mrf.h"
#include "power.h"
#include "sdcard.h"
#include "sleep.h"
#include "tsc.h"
#include "wheels.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_CHANNEL 20
#define DEFAULT_PAN 0x1846
#define DEFAULT_INDEX 0

#define BATTERY_AVERAGE_FACTOR 8
#define BATTERY_DIVIDER_TOP 20000.0
#define BATTERY_DIVIDER_BOTTOM 2200.0
#define BATTERY_VOLTS_PER_LSB (3.3 / 1024.0 / BATTERY_DIVIDER_BOTTOM * (BATTERY_DIVIDER_TOP + BATTERY_DIVIDER_BOTTOM))
#define BATTERY_LOW_THRESHOLD 12.5

#define HIGH_TEMPERATURE_THRESHOLD 401 /* See software/util/thermal.cpp for why this is equivalent to 100°C. */
#define BREAKBEAM_DIFF_THRESHOLD 15

#define SPI_FLASH_SIZE (16UL / 8UL * 1024UL * 1024UL)
#define SPI_FLASH_PARAMETERS_ADDRESS (SPI_FLASH_SIZE - 4096UL)

static unsigned char stack[1024] __attribute__((section(".stack"), used));

static void entry(void) __attribute__((section(".entry"), used, naked));
static void entry(void) {
	// Initialize the stack pointer and jump to avr_main
	asm volatile(
		"ldi r16, 0xFF\n\t"
		"out 0x3D, r16\n\t"
		"ldi r16, 0x0C\n\t"
		"out 0x3E, r16\n\t"
		"jmp avr_main\n\t");
}

static uint8_t channel = DEFAULT_CHANNEL, index = DEFAULT_INDEX;
static uint16_t pan = DEFAULT_PAN;
static bool feedback_pending = false;
static uint8_t tx_seqnum = 0;
static uint16_t rx_seqnum = 0xFFFF;
static uint8_t led_mode = 0x20;
static uint16_t autokick_pulse_width;
static uint8_t autokick_device;
static bool autokick_armed = false;
static bool autokick_fired_pending = false;
static bool drivetrain_power_forced_on = false;
static uint32_t last_drive_packet_time = 0;
static uint16_t battery_average;
static bool radio_tx_packet_prepared = false, radio_tx_packet_reliable;
static bool log_overflow_feedback_report_pending = false;

static void shutdown_sequence(void) __attribute__((noreturn));
static void shutdown_sequence(void) {
	set_charge_mode(false);
	set_discharge_mode(true);
	motor_scram();
	log_deinit();
	sleep_1s();
	sleep_1s();
	sleep_1s();
	POWER_CTL = 0x01;
	sleep_1s();
	POWER_CTL = 0x00;
	for (;;);
}

static void prepare_mrf_mhr(uint8_t payload_length) {
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
	mrf_tx_buffer[0] = 9;
	mrf_tx_buffer[1] = 9 + payload_length; // Frame length
	mrf_tx_buffer[2] = 0b01100001; // Frame control LSB
	mrf_tx_buffer[3] = 0b10001000; // Frame control MSB
	mrf_tx_buffer[4] = tx_seqnum++; // Sequence number
	mrf_tx_buffer[5] = pan & 0xFF; // Destination PAN ID LSB
	mrf_tx_buffer[6] = pan >> 8; // Destination PAN ID MSB
	mrf_tx_buffer[7] = 0x00; // Destination address LSB
	mrf_tx_buffer[8] = 0x01; // Destination address MSB
	mrf_tx_buffer[9] = index; // Source address LSB
	mrf_tx_buffer[10] = 0; // Source address MSB
}

static void prepare_feedback_packet(void) {
	prepare_mrf_mhr(14);

#define payload (&mrf_tx_buffer[11])
	payload[0] = 0x00; // General robot status update
	payload[1] = battery_average; // Battery voltage LSB
	payload[2] = battery_average >> 8; // Battery voltage MSB
	uint16_t adc_value = read_main_adc(CHICKER);
	payload[3] = adc_value; // Capacitor voltage LSB
	payload[4] = adc_value >> 8; // Capacitor voltage MSB
	int16_t breakbeam_diff = read_breakbeam_diff();
	payload[5] = breakbeam_diff; // Break beam reading LSB
	payload[6] = breakbeam_diff >> 8; // Break beam reading MSB
	adc_value = read_main_adc(TEMPERATURE);
	payload[7] = adc_value; // Board temperature LSB
	payload[8] = adc_value >> 8; // Board temperature MSB
	uint8_t flags = 0;
	if (breakbeam_diff < BREAKBEAM_DIFF_THRESHOLD) {
		flags |= 0x01;
	}
	if (is_charged()) {
		flags |= 0x02;
	}
	if (is_charge_timeout()) {
		flags |= 0x04;
	}
	if (POWER_CTL & 0x10 /* Breakout present */) {
		flags |= 0x08;
	}
	if (CHICKER_CTL & 0x40 /* Chicker present */) {
		flags |= 0x10;
	}
	if (interlocks_overridden()) {
		flags |= 0x40;
	}
	payload[9] = flags; // Flags
	flags = 0;
	for (uint8_t wheel = 0; wheel < 4; ++wheel) {
		MOTOR_INDEX = wheel;
		uint8_t status = MOTOR_STATUS;
		MOTOR_STATUS = ~status;
		flags |= status << (wheel * 2);
	}
	payload[10] = flags; // Wheel motor Hall sensors stuck
	MOTOR_INDEX = 4;
	{
		uint8_t motor_status = MOTOR_STATUS;
		MOTOR_STATUS = ~motor_status;
		flags = motor_status | (ENCODER_FAIL << 2);
	}
	payload[11] = flags; // Dribbler Hall sensors stuck and optical encoders not commutating
	if (log_overflow_feedback_report_pending) {
		payload[12] = (LOG_STATE_OVERFLOW << 4) | SD_STATUS_OK;
		log_overflow_feedback_report_pending = false;
	} else {
		payload[12] = (log_state() << 4) | sd_status();
	}
	payload[13] = dribbler_speed;
#undef payload

	radio_tx_packet_reliable = false;
	radio_tx_packet_prepared = true;
}

static void prepare_autokick_packet(void) {
	prepare_mrf_mhr(1);

#define payload (&mrf_tx_buffer[11])
	payload[0] = 0x01; // Autokick fired
#undef payload

	radio_tx_packet_reliable = true;
	radio_tx_packet_prepared = true;
}

static void handle_radio_receive(void) {
	uint8_t frame_length = mrf_rx_buffer[0];
	uint16_t frame_control = mrf_rx_buffer[1] | (mrf_rx_buffer[2] << 8);
	// Sanity-check the frame control word
	if (((frame_control >> 0) & 7) == 1 /* Data packet */ && ((frame_control >> 3) & 1) == 0 /* No security */ && ((frame_control >> 6) & 1) == 1 /* Intra-PAN */ && ((frame_control >> 10) & 3) == 2 /* 16-bit destination address */ && ((frame_control >> 14) & 3) == 2 /* 16-bit source address */) {
		// Read out and check the source address and sequence number
		uint16_t source_address = mrf_rx_buffer[8] | (mrf_rx_buffer[9] << 8);
		uint8_t sequence_number = mrf_rx_buffer[3];
		if (source_address == 0x0100 && sequence_number != rx_seqnum) {
			// Update sequence number
			rx_seqnum = sequence_number;

			// Handle packet
			uint16_t dest_address = mrf_rx_buffer[6] | (mrf_rx_buffer[7] << 8);
			static const uint8_t HEADER_LENGTH = 2 /* Frame control */ + 1 /* Seq# */ + 2 /* Dest PAN */ + 2 /* Dest */ + 2 /* Src */;
			static const uint8_t FOOTER_LENGTH = 2;
			if (dest_address == 0xFFFF) {
				// Broadcast frame must contain drive packet, which must be 64 bytes long
				if (frame_length == HEADER_LENGTH + 64 + FOOTER_LENGTH) {
					// Construct the individual 16-bit words sent from the host.
					last_drive_packet_time = rdtsc();
					const uint8_t offset = 1 + HEADER_LENGTH + 8 * index;
					uint16_t words[4];
					for (uint8_t i = 0; i < 4; ++i) {
						words[i] = mrf_rx_buffer[offset + i * 2 + 1];
						words[i] <<= 8;
						words[i] |= mrf_rx_buffer[offset + i * 2];
					}

					// Check for feedback request.
					feedback_pending = !!(words[0] & 0x8000);

					// Set new wheel setpoints and mode.
					{
						int16_t new_setpoints[4];
						for (uint8_t i = 0; i < 4; ++i) {
							new_setpoints[i] = words[i] & 0x3FF;
							if (words[i] & 0x400) {
								new_setpoints[i] = -new_setpoints[i];
							}
						}
						switch ((words[0] >> 13) & 0b11) {
							case 0b00: wheels_mode = WHEELS_MODE_MANUAL_COMMUTATION; break;
							case 0b01: wheels_mode = WHEELS_MODE_BRAKE; break;
							case 0b10: wheels_mode = WHEELS_MODE_OPEN_LOOP; break;
							case 0b11: wheels_mode = WHEELS_MODE_CLOSED_LOOP; break;
						}
						if (wheels_mode == WHEELS_MODE_CLOSED_LOOP) {
							control_process_new_setpoints(new_setpoints);
						} else {
							memcpy(wheels_setpoints.wheels, new_setpoints, sizeof(new_setpoints));
						}
					}

					// Set new dribbler mode.
					// Delay sending new mode to motors until after evaluating drivetrain power to avoid possible latchup in motor drivers if drivetrain is unpowered.
					dribbler_enabled = !!(words[0] & (1 << 12));

					// Set new chicker mode.
					switch ((words[1] >> 14) & 3) {
						case 0b00:
							set_charge_mode(false);
							set_discharge_mode(false);
							break;
						case 0b01:
							set_charge_mode(false);
							set_discharge_mode(true);
							break;
						case 0b10:
							set_discharge_mode(false);
							set_charge_mode(true);
							break;
					}

					// Turn drivetrain power on or off if needed.
					if (drivetrain_power_forced_on || wheels_mode != WHEELS_MODE_MANUAL_COMMUTATION || dribbler_enabled) {
						power_enable_motors();
					} else {
						power_disable_motors();
					}
				}
			} else if (frame_length >= HEADER_LENGTH + 1 + FOOTER_LENGTH) {
				// Non-broadcast frame contains a message specifically for this robot
				static const uint16_t MESSAGE_PURPOSE_ADDR = 1 + HEADER_LENGTH;
				static const uint16_t MESSAGE_PAYLOAD_ADDR = MESSAGE_PURPOSE_ADDR + 1;
				switch (mrf_rx_buffer[MESSAGE_PURPOSE_ADDR]) {
					case 0x00: // Fire kicker immediately
						if (frame_length == HEADER_LENGTH + 4 + FOOTER_LENGTH) {
							uint8_t which = mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR];
							uint16_t width = mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR + 2];
							width <<= 8;
							width |= mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR + 1];
							set_chick_pulse(width);
							if (which) {
								fire_chipper();
							} else {
								fire_kicker();
							}
						}
						break;

					case 0x01: // Arm autokick
						if (frame_length == HEADER_LENGTH + 4 + FOOTER_LENGTH) {
							autokick_device = mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR];
							autokick_pulse_width = mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR + 2];
							autokick_pulse_width <<= 8;
							autokick_pulse_width |= mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR + 1];
							autokick_armed = true;
						}
						break;

					case 0x02: // Disarm autokick
						if (frame_length == HEADER_LENGTH + 1 + FOOTER_LENGTH) {
							autokick_armed = false;
						}
						break;

					case 0x03: // Set LED mode
						if (frame_length == HEADER_LENGTH + 2 + FOOTER_LENGTH) {
							led_mode = mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR];
							if (led_mode <= 0x1F) {
								LED_CTL = (LED_CTL & 0x80) | led_mode;
							} else if (led_mode == 0x21) {
								set_test_leds(USER_MODE, 7);
							} else {
								set_test_leds(USER_MODE, 0);
							}
						}
						break;

					case 0x08: // Reboot
						if (frame_length == HEADER_LENGTH + 1 + FOOTER_LENGTH) {
							power_reboot();
						}
						break;

					case 0x09: // Force on motor power
						drivetrain_power_forced_on = true;
						break;

					case 0x0B: // Set bootup radio parameters
						if (frame_length == HEADER_LENGTH + 5 + FOOTER_LENGTH) {
							// Extract the new data from the radio
							uint8_t buffer[4];
							for (uint8_t i = 0; i < sizeof(buffer); ++i) {
								buffer[i] = mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR + i];
							}

							// Erase and rewrite the sector
							flash_erase_sector(SPI_FLASH_PARAMETERS_ADDRESS);
							flash_page_program(SPI_FLASH_PARAMETERS_ADDRESS >> 8, buffer, sizeof(buffer));

							puts("Parameters rewritten.");
						}
						break;

					case 0x0C: // Shut down
						{
							puts("Shutting down at operator request.");
							shutdown_sequence();
						}

					case 0x0D: // Manually commutate motors
						if (frame_length == HEADER_LENGTH + 6 + FOOTER_LENGTH) {
							for (uint8_t i = 0; i < 5; ++i) {
								motor_manual_commutation_patterns[i] = mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR + i] & 0b11111100;
							}
						}
						break;
				}
			}
		}
	}
}

static void handle_tick(void) {
	// Compute battery voltage in actual volts.
	float battery_volts = battery_average * BATTERY_VOLTS_PER_LSB;

	// Run the wheels.
	wheels_tick(battery_volts);

	// Run the dribbler.
	dribbler_tick(battery_volts);

	// Update IIR filter on battery voltage and check for low battery.
	battery_average = (battery_average * (BATTERY_AVERAGE_FACTOR - 1) + read_main_adc(BATT_VOLT) + (BATTERY_AVERAGE_FACTOR / 2)) / BATTERY_AVERAGE_FACTOR;
	if (battery_average < (unsigned int) (BATTERY_LOW_THRESHOLD / BATTERY_VOLTS_PER_LSB) && !interlocks_overridden()) {
		puts("Shutting down due to low battery.");
		shutdown_sequence();
	}

	// Write a log record if possible.
	log_record_t *rec = log_alloc();
	if (rec) {
		rec->magic = LOG_MAGIC_TICK;

		rec->tick.breakbeam_diff = read_breakbeam_diff();
		for (uint8_t i = 0; i < sizeof(rec->tick.adc_channels) / sizeof(*rec->tick.adc_channels); ++i) {
			rec->tick.adc_channels[i] = read_main_adc(i);
		}

		if (wheels_mode == WHEELS_MODE_CLOSED_LOOP) {
			for (uint8_t i = 0; i < 3; ++i) {
				rec->tick.wheels_setpoints[i] = wheels_setpoints.robot[i];
			}
			rec->tick.wheels_setpoints[3] = 0;
		} else {
			for (uint8_t i = 0; i < 4; ++i) {
				rec->tick.wheels_setpoints[i] = wheels_setpoints.wheels[i];
			}
		}
		for (uint8_t i = 0; i < sizeof(rec->tick.wheels_encoder_counts) / sizeof(*rec->tick.wheels_encoder_counts); ++i) {
			rec->tick.wheels_encoder_counts[i] = wheels_encoder_counts[i];
		}
		for (uint8_t i = 0; i < sizeof(rec->tick.wheels_drives) / sizeof(*rec->tick.wheels_drives); ++i) {
			rec->tick.wheels_drives[i] = wheels_drives[i];
		}

		rec->tick.encoders_failed = ENCODER_FAIL;
		rec->tick.wheels_hall_sensors_failed = 0;
		for (uint8_t wheel = 0; wheel < 4; ++wheel) {
			MOTOR_INDEX = wheel;
			rec->tick.wheels_hall_sensors_failed |= MOTOR_STATUS << (2 * wheel);
		}
		MOTOR_INDEX = 4;
		rec->tick.dribbler_hall_sensors_failed = MOTOR_STATUS;

		rec->tick.cpu_used_since_last_tick = cpu_usage;
		cpu_usage = 0;

		log_commit();
	} else if (log_state() == LOG_STATE_OVERFLOW) {
		log_overflow_feedback_report_pending = true;
	}
}

static void avr_main(void) __attribute__((noreturn, used));
static void avr_main(void) {
	// Initialize the debug port.
	debug_init();

	// Print the device DNA on the debug port.
	{
		uint64_t device_dna = device_dna_read();
		sleep_short();
		printf("\n\nDevice DNA: %06" PRIX32 "%08" PRIX32 "\n", (uint32_t) (device_dna >> 32), (uint32_t) device_dna);
	}

	// Read the parameters from the end of the SPI flash.
	{
		uint8_t buffer[4];
		flash_read(SPI_FLASH_PARAMETERS_ADDRESS, buffer, sizeof(buffer));
		fputs("Parameters block:", stdout);
		for (uint8_t i = 0; i < sizeof(buffer); ++i) {
			printf("0x%" PRIX8 " ", buffer[i]);
		}
		putchar('\n');
		if ((0x0B <= buffer[0] && buffer[0] <= 0x1A) && (buffer[1] <= 7) && ((buffer[2] != 0xFF) || (buffer[3] != 0xFF))) {
			puts("Parameters OK.");
			channel = buffer[0];
			index = buffer[1];
			pan = buffer[3];
			pan <<= 8;
			pan |= buffer[2];
		} else {
			puts("Invalid parameters; using defaults.");
		}
	}

	// Initialize the SD card and the logging layer.
	if (sd_init()) {
		log_init();
	}

	// Initialize the radio.
	mrf_init(channel, false, pan, index, UINT64_C(0xec89d61e8ffd409b));

	// Turn on the radio LED.
	radio_led_ctl(true);

	// Initialize the battery average to the current level.
	battery_average = read_main_adc(BATT_VOLT);

	// Initialize a tick count.
	uint32_t last_control_loop_time = rdtsc();
	for(;;) {
		// Check if an autokick needs to fire.
		if (autokick_armed && read_breakbeam_diff() < BREAKBEAM_DIFF_THRESHOLD) {
			set_chick_pulse(autokick_pulse_width);
			if (autokick_device) {
				fire_chipper();
			} else {
				fire_kicker();
			}
			autokick_armed = false;
			autokick_fired_pending = true;
		}

		// Check if a tick has passed; if so, iterate the control loop, update average battery level, and log a tick record.
		{
			uint32_t now = rdtsc();
			if (now - last_control_loop_time >= (F_CPU / CONTROL_LOOP_HZ)) {
				last_control_loop_time = now;
				handle_tick();
				cpu_usage += rdtsc() - now;
			}
		}

		// Check if a radio packet has been received.
		if (mrf_rx_poll()) {
			uint32_t start = rdtsc();
			radio_led_blink();
			handle_radio_receive();
			cpu_usage += rdtsc() - start;
		}

		// How the radio transmit path works is that packets that need transmitting go through three different states:
		// 1. When an event occurs that triggers the need to transmit, the packet becomes PENDING:
		//     Its *_pending variable is set to true.
		// 2. When the transmit buffer in RAM is free and no packet has been prepared yet, one of the PENDING packets becomes PREPARED:
		//     Its *_pending variable is set to false.
		//     Its data is generated in mrf_tx_buffer.
		//     The radio_tx_packet_prepared variable is set to true.
		// 3. When the transmit path in the radio is free, the PREPARED packet becomes TRANSMITTING:
		//     A radio transmit operation is started.
		//     The radio_tx_packet_prepared variable is set to false.

		// Check if we should assemble a packet now.
		if (mrf_tx_buffer_free()) {
			if (feedback_pending) {
				uint32_t start = rdtsc();
				prepare_feedback_packet();
				feedback_pending = false;
				cpu_usage += rdtsc() - start;
			} else if (autokick_fired_pending) {
				uint32_t start = rdtsc();
				prepare_autokick_packet();
				autokick_fired_pending = false;
				cpu_usage += rdtsc() - start;
			}
		}

		// Check if a packet has been prepared and the MRF transmit path is ready to transmit it.
		if (radio_tx_packet_prepared && mrf_tx_path_free()) {
			uint32_t start = rdtsc();
			mrf_tx_start(radio_tx_packet_reliable);
			radio_tx_packet_prepared = false;
			cpu_usage += rdtsc() - start;
		}

		// Update the LEDs.
		if (led_mode == 0x20) {
			uint8_t flags = 0;
			if (read_breakbeam_diff() < BREAKBEAM_DIFF_THRESHOLD) {
				flags |= 0x01;
			}
			if (autokick_armed) {
				flags |= 0x04;
			}
			set_test_leds(USER_MODE, flags);
		}

		// Check if more than a second has passed since we last received a fresh drive packet.
		if (rdtsc() - last_drive_packet_time > F_CPU) {
			// Time out and stop driving.
			uint32_t start = rdtsc();
			memset(&wheels_setpoints, 0, sizeof(wheels_setpoints));
			wheels_mode = WHEELS_MODE_MANUAL_COMMUTATION;
			dribbler_enabled = false;
			set_charge_mode(false);
			set_discharge_mode(false);
			cpu_usage += rdtsc() - start;
		}

		// Check if the board temperature is over 100°C and interlocks are enabled.
		if (read_main_adc(TEMPERATURE) < HIGH_TEMPERATURE_THRESHOLD && !interlocks_overridden()) {
			// Turn off all the motors.
			uint32_t start = rdtsc();
			memset(&wheels_setpoints, 0, sizeof(wheels_setpoints));
			wheels_mode = WHEELS_MODE_MANUAL_COMMUTATION;
			dribbler_enabled = false;
			motor_scram();
			cpu_usage += rdtsc() - start;
		}
	}
}

