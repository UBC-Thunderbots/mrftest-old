#include "adc.h"
#include "buffers.h"
#include "chicker.h"
#include "debug.h"
#include "device_dna.h"
#include "flash.h"
#include "io.h"
#include "led.h"
#include "motor.h"
#include "mrf.h"
#include "power.h"
#include "sdcard.h"
#include "sleep.h"
#include "wheels.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_CHANNEL 20
#define DEFAULT_PAN 0x1846
#define DEFAULT_INDEX 0

#define LOW_BATTERY_THRESHOLD ((unsigned int) (12.5 / (10.0e3 * 2 + 2.2e3) * 2.2e3 / 3.3 * 1024.0))
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
		"rjmp avr_main\n\t");
}

static uint8_t channel = DEFAULT_CHANNEL, index = DEFAULT_INDEX;
static uint16_t pan = DEFAULT_PAN;
static bool transmit_busy = false, transmission_reliable = false, feedback_pending = false;
static uint8_t tx_seqnum = 0;
static uint16_t rx_seqnum = 0xFFFF;
static uint8_t led_mode = 0x20;
static uint16_t autokick_pulse_width;
static uint8_t autokick_device;
static bool autokick_armed = false;
static bool autokick_fired_pending = false;
static bool drivetrain_power_forced_on = false;
static uint8_t last_drive_packet_tick = 0;
static uint16_t battery_average = 1023;
static bool mrf_rx_dma_started = false;
static uint32_t ticks32 = 0;

static void shutdown_sequence(void) __attribute__((noreturn));
static void shutdown_sequence(void) {
	set_charge_mode(false);
	set_discharge_mode(true);
	motor_scram();
	sd_write_multi_end();
	sleep_1s();
	sleep_1s();
	sleep_1s();
	POWER_CTL = 0x01;
	sleep_1s();
	POWER_CTL = 0x00;
	for (;;);
}

void send_current_packet_and_fix_frame_length_for_logging(void) {
	const uint8_t *pbuf = next_packet_buffer;
	mrf_write_long(MRF_REG_LONG_TXNFIFO, 9); // Header length (used by MRF24J40 but not logged to SD)
	uint8_t frame_length = *pbuf + 1; // Include frame length byte in frame length so enough bytes will be copied to MRF
	uint16_t mrf_reg_index = MRF_REG_LONG_TXNFIFO + 1;
	do {
		mrf_write_long(mrf_reg_index++, *pbuf++);
	} while (--frame_length);
	mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);
	transmit_busy = true;
	next_packet_buffer[0] += 2; // Add 2 bytes for (non-existent) FCS in the log, so structure matches that of RX packet
}

void prepare_mrf_mhr(uint8_t payload_length) {
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
	uint8_t *pbuf = next_packet_buffer;
	pbuf[0] = 9 + payload_length; // Frame length
	pbuf[1] = 0b01100001; // Frame control LSB
	pbuf[2] = 0b10001000; // Frame control MSB
	pbuf[3] = tx_seqnum++; // Sequence number
	pbuf[4] = pan & 0xFF; // Destination PAN ID LSB
	pbuf[5] = pan >> 8; // Destination PAN ID MSB
	pbuf[6] = 0x00; // Destination address LSB
	pbuf[7] = 0x01; // Destination address MSB
	pbuf[8] = index; // Source address LSB
	pbuf[9] = 0; // Source address MSB
}

static void send_feedback_packet(void) {
	prepare_mrf_mhr(12);

	uint8_t *payload = next_packet_buffer + 10;
	payload[0] = 0x00; // General robot status update
	uint16_t adc_value = read_main_adc(BATT_VOLT);
	payload[1] = adc_value; // Battery voltage LSB
	payload[2] = adc_value >> 8; // Battery voltage MSB
	adc_value = read_main_adc(CHICKER);
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
	if (SD_CTL & 0x02 /* SD card present */) {
		flags |= 0x20;
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

	send_current_packet_and_fix_frame_length_for_logging();
	feedback_pending = false;
	transmission_reliable = false;
}

static void handle_radio_receive(void) {
	mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
	uint8_t frame_length = next_packet_buffer[0];
	uint16_t frame_control = next_packet_buffer[1] | (next_packet_buffer[2] << 8);
	// Sanity-check the frame control word
	if (((frame_control >> 0) & 7) == 1 /* Data packet */ && ((frame_control >> 3) & 1) == 0 /* No security */ && ((frame_control >> 6) & 1) == 1 /* Intra-PAN */ && ((frame_control >> 10) & 3) == 2 /* 16-bit destination address */ && ((frame_control >> 14) & 3) == 2 /* 16-bit source address */) {
		// Read out and check the source address and sequence number
		uint16_t source_address = next_packet_buffer[8] | (next_packet_buffer[9] << 8);
		uint8_t sequence_number = next_packet_buffer[3];
		if (source_address == 0x0100 && sequence_number != rx_seqnum) {
			// Update sequence number
			rx_seqnum = sequence_number;

			// Handle packet
			uint16_t dest_address = next_packet_buffer[6] | (next_packet_buffer[7] << 8);
			static const uint8_t HEADER_LENGTH = 2 /* Frame control */ + 1 /* Seq# */ + 2 /* Dest PAN */ + 2 /* Dest */ + 2 /* Src */;
			static const uint8_t FOOTER_LENGTH = 2;
			if (dest_address == 0xFFFF) {
				// Broadcast frame must contain drive packet, which must be 64 bytes long
				if (frame_length == HEADER_LENGTH + 64 + FOOTER_LENGTH) {
					last_drive_packet_tick = TICKS;
					const uint8_t offset = 1 + HEADER_LENGTH + 8 * index;
					uint16_t words[4];
					for (uint8_t i = 0; i < 4; ++i) {
						words[i] = next_packet_buffer[offset + i * 2 + 1];
						words[i] <<= 8;
						words[i] |= next_packet_buffer[offset + i * 2];
						wheel_context.setpoints[i] = words[i] & 0x3FF;
						if (words[i] & 0x400) {
							wheel_context.setpoints[i] = -wheel_context.setpoints[i];
						}
					}
					feedback_pending = !!(words[0] & 0x8000);
					switch ((words[0] >> 13) & 0b11) {
						case 0b00: wheel_context.mode = WHEEL_MODE_MANUAL_COMMUTATION; break;
						case 0b01: wheel_context.mode = WHEEL_MODE_BRAKE; break;
						case 0b10: wheel_context.mode = WHEEL_MODE_OPEN_LOOP; break;
						case 0b11: wheel_context.mode = WHEEL_MODE_CLOSED_LOOP; break;
					}

					wheel_update_ctx();

					if (drivetrain_power_forced_on || wheel_context.mode != WHEEL_MODE_MANUAL_COMMUTATION || words[0] & (1 << 12)) {
						power_enable_motors();
					} else {
						power_disable_motors();
					}

					if (words[0] & (1 << 12)) {
						set_dribbler(FORWARD, 180);
					} else {
						set_dribbler(MANUAL_COMMUTATION, 0);
					}

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
				}
			} else if (frame_length >= HEADER_LENGTH + 1 + FOOTER_LENGTH) {
				// Non-broadcast frame contains a message specifically for this robot
				static const uint16_t MESSAGE_PURPOSE_ADDR = 1 + HEADER_LENGTH;
				static const uint16_t MESSAGE_PAYLOAD_ADDR = MESSAGE_PURPOSE_ADDR + 1;
				switch (next_packet_buffer[MESSAGE_PURPOSE_ADDR]) {
					case 0x00: // Fire kicker immediately
						if (frame_length == HEADER_LENGTH + 4 + FOOTER_LENGTH) {
							uint8_t which = next_packet_buffer[MESSAGE_PAYLOAD_ADDR];
							uint16_t width = next_packet_buffer[MESSAGE_PAYLOAD_ADDR + 2];
							width <<= 8;
							width |= next_packet_buffer[MESSAGE_PAYLOAD_ADDR + 1];
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
							autokick_device = next_packet_buffer[MESSAGE_PAYLOAD_ADDR];
							autokick_pulse_width = next_packet_buffer[MESSAGE_PAYLOAD_ADDR + 2];
							autokick_pulse_width <<= 8;
							autokick_pulse_width |= next_packet_buffer[MESSAGE_PAYLOAD_ADDR + 1];
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
							led_mode = next_packet_buffer[MESSAGE_PAYLOAD_ADDR];
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
								buffer[i] = next_packet_buffer[MESSAGE_PAYLOAD_ADDR + i];
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
								motor_manual_commutation_patterns[i] = next_packet_buffer[MESSAGE_PAYLOAD_ADDR + i] & 0b11111100;
							}
						}
						break;
				}
			}
		}
	}
}

static void avr_main(void) __attribute__((noreturn, used));
static void avr_main(void) {
	// Initialize the debug port
	debug_init();

	// Print the device DNA on the debug port
	{
		uint64_t device_dna = device_dna_read();
		sleep_short();
		printf("\n\nDevice DNA: %06" PRIX32 "%08" PRIX32 "\n", (uint32_t) (device_dna >> 32), (uint32_t) device_dna);
	}

	// Read the parameters from the end of the SPI flash
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

	// Initialize the SD card logging layer.
	memset(buffers, 0, sizeof(buffers));
	if (sd_init()) {
		uint32_t low = 0, high = sd_sector_count();
		while (low != high && sd_status() == SD_STATUS_OK) {
			uint32_t probe = (low + high) / 2;
			if (sd_read(probe, &buffers[0])) {
				if (buffers[0].tick.epoch) {
					low = probe + 1;
				} else {
					high = probe;
				}
			}
		}
		if (low > 0) {
			if (sd_read(low - 1, &buffers[0])) {
				buffers[0].tick.epoch++;
				buffers[1].tick.epoch = buffers[0].tick.epoch;
			}
		} else {
			buffers[0].tick.epoch = 1;
			buffers[1].tick.epoch = 1;
		}
		if (sd_status() == SD_STATUS_OK) {
			if (sd_write_multi_start(low)) {
				printf("Starting log at sector %" PRIu32 " with epoch %" PRIu16 "\n", low, buffers[0].tick.epoch);
			}
		}
	}

	// Initialize the radio
	mrf_init();
	sleep_short();
	mrf_release_reset();
	sleep_short();
	mrf_common_init(channel, false, pan, UINT64_C(0xec89d61e8ffd409b));
	while (MRF_CTL & 0x10);
	mrf_write_short(MRF_REG_SHORT_SADRH, 0);
	mrf_write_short(MRF_REG_SHORT_SADRL, index);
	mrf_analogue_txrx();
	mrf_write_short(MRF_REG_SHORT_INTCON, 0b11110110);

	// Turn on the radio LED
	radio_led_ctl(true);

	// Initialize a tick count
	uint8_t old_ticks = TICKS;
	for(;;) {
		// Check if an autokick needs to fire
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

		// Check if a tick has passed; if so, iterate the control loop and update average battery level.
		// We must not run a tick if an MRF receive is running, because the SD and MRF DMA operations would overlap and break.
		// MRF receive should take very little time, so this should not significantly delay ticks.
		{
			uint8_t new_ticks = TICKS;
			if (new_ticks != old_ticks && !mrf_rx_dma_started) {
				ticks32 += (uint8_t) (new_ticks - old_ticks);
				old_ticks = new_ticks;
				current_buffer->tick.encoders_failed = ENCODER_FAIL;
				current_buffer->tick.wheel_hall_sensors_failed = 0;
				for (uint8_t wheel = 0; wheel < 4; ++wheel) {
					MOTOR_INDEX = wheel;
					current_buffer->tick.wheel_hall_sensors_failed |= MOTOR_STATUS << (2 * wheel);
				}
				MOTOR_INDEX = 4;
				current_buffer->tick.dribbler_hall_sensors_failed = MOTOR_STATUS;
				wheels_tick();
				battery_average = (battery_average * 63 + read_main_adc(BATT_VOLT)) / 64;
				if (battery_average < LOW_BATTERY_THRESHOLD && !interlocks_overridden()) {
					puts("Shutting down due to low battery.");
					shutdown_sequence();
				}
				if (sd_write_multi_active() && !sd_write_multi_busy()) {
					current_buffer->tick.ticks = ticks32;
					current_buffer->tick.breakbeam_diff = read_breakbeam_diff();
					for (uint8_t i = 0; i < 8; ++i) {
						current_buffer->tick.adc_channels[i] = read_main_adc(i);
					}
					sd_write_multi_sector(current_buffer);
					buffers_swap();
					uint16_t epoch = current_buffer->tick.epoch;
					memset(current_buffer, 0, sizeof(buffer_t));
					current_buffer->tick.epoch = epoch;
				}
			}
		}

		// Check if there is activity on the radio that needs handling
		if (mrf_get_interrupt()) {
			// See what happened
			uint8_t intstat = mrf_read_short(MRF_REG_SHORT_INTSTAT);
			if (intstat & (1 << 0)) {
				// Transmission complete; check status
				uint8_t txstat = mrf_read_short(MRF_REG_SHORT_TXSTAT);
				if ((txstat & 0x01) && transmission_reliable) {
					// Transmission failed; resubmit frame
					mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);
				} else {
					// Transmission succeeded; release radio
					transmit_busy = false;
				}
			}
			if ((intstat & (1 << 3)) && !mrf_rx_dma_started) {
				// Packet received.
				// Blink a light.
				radio_led_blink();

				// Disable reception of further packets.
				mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception

				// Start copying the data into the receive buffer.
				mrf_start_read_long_block(MRF_REG_LONG_RXFIFO, next_packet_buffer, 128);
				mrf_rx_dma_started = true;
			}
		}

		// Check if a DMA transfer from the radio has now finished and is ready to handle the received packet.
		if (mrf_rx_dma_started && !mrf_read_long_block_active()) {
			mrf_rx_dma_started = false;
			handle_radio_receive();
			buffers_push_packet();
		}

		// Check if we should send a feedback packet now
		if (feedback_pending && !transmit_busy) {
			send_feedback_packet();
		}

		// Update the LEDs
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

		// Check if the autokick system fired and the report needs transmitting
		if (autokick_fired_pending && !transmit_busy) {
			// Send notification to host
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
			prepare_mrf_mhr(1);
			uint8_t *payload = next_packet_buffer + 10;
			payload[0] = 0x01; // Autokick fired
			send_current_packet_and_fix_frame_length_for_logging();
			autokick_fired_pending = false;
			transmission_reliable = true;
		}

		// Check if more than a second has passed since we last received a fresh drive packet
		if ((uint8_t) (TICKS - last_drive_packet_tick) > 200) {
			// Time out and stop driving
			for (uint8_t i = 0; i < 4; ++i) {
				wheel_context.setpoints[i] = 0;
			}
			wheel_context.mode = WHEEL_MODE_MANUAL_COMMUTATION;
			wheel_update_ctx();
			set_dribbler(MANUAL_COMMUTATION, 0);
			set_charge_mode(false);
			set_discharge_mode(false);
		}
	}
}

