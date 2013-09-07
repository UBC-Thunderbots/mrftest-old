#include "control.h"
#include "device_dna.h"
#include "dribbler.h"
#include "flash.h"
#include "globals.h"
#include "io.h"
#include "log.h"
#include "motor.h"
#include "mrf.h"
#include "sdcard.h"
#include "sleep.h"
#include "syscalls.h"
#include "wheels.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void);
extern char linker_data_vma_start;
extern char linker_data_vma_end;
extern char linker_data_lma_start;
extern char linker_bss_vma_start;
extern char linker_bss_vma_end;

static void * const VECTORS[] __attribute__((section(".vectors"), used)) = {
	(void *) 0x55AA9966,
	&main,
	&linker_data_vma_start,
	&linker_data_vma_end,
	&linker_data_lma_start,
	&linker_bss_vma_start,
	&linker_bss_vma_end,
};

#define DEFAULT_CHANNEL 11
#define DEFAULT_INDEX 0
#define DEFAULT_PAN 0x1846

#define BATTERY_AVERAGE_FACTOR 200
#define BATTERY_DIVIDER_TOP 20000.0
#define BATTERY_DIVIDER_BOTTOM 2200.0
#define BATTERY_VOLTS_PER_LSB (3.3 / 1024.0 / BATTERY_DIVIDER_BOTTOM * (BATTERY_DIVIDER_TOP + BATTERY_DIVIDER_BOTTOM))
#define BATTERY_LOW_THRESHOLD 12.5

#define HIGH_TEMPERATURE_THRESHOLD 401 /* See software/util/thermal.cpp for why this is equivalent to 100°C. */
#define BREAKBEAM_DIFF_THRESHOLD 15

static uint8_t channel = DEFAULT_CHANNEL, robot_index = DEFAULT_INDEX;
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
static float battery_average;
static bool radio_tx_packet_prepared = false, radio_tx_packet_reliable, radio_has_prepared_feedback = false, radio_was_sending_feedback = false;
static bool log_overflow_feedback_report_pending = false;
static uint8_t last_drive_data_serial = 0xFF;
static bool last_breakbeam_report = false;
static uint8_t radio_pending_motor_sensor_failures[5], radio_reporting_motor_sensor_failures[5];
static unsigned int radio_last_blink_time;

static void shutdown_sequence(bool reboot) __attribute__((noreturn));
static void shutdown_sequence(bool reboot) {
	{
		io_chicker_csr_t csr = { 0 };
		csr.discharge = true;
		IO_CHICKER.csr = csr;
	}
	motor_scram();
	log_deinit();
	sleep_1s();
	IO_SYSCTL.csr.motor_power = 0;
	sleep_1s();
	if (reboot) {
		IO_SYSCTL.csr.amba_reset = 1;
	} else {
		IO_SYSCTL.csr.logic_power = 0;
	}
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
	mrf_tx_buffer[9] = robot_index; // Source address LSB
	mrf_tx_buffer[10] = 0; // Source address MSB
}

static void prepare_feedback_packet(void) {
	prepare_mrf_mhr(16);

#define payload (&mrf_tx_buffer[11])
	payload[0] = 0x00; // General robot status update

	payload[1] = battery_average / BATTERY_VOLTS_PER_LSB; // Battery voltage LSB
	payload[2] = ((unsigned int) (battery_average / BATTERY_VOLTS_PER_LSB)) >> 8; // Battery voltage MSB

	uint16_t adc_value = IO_CHICKER.capacitor_voltage;
	payload[3] = adc_value; // Capacitor voltage LSB
	payload[4] = adc_value >> 8; // Capacitor voltage MSB

	int16_t breakbeam_diff = IO_SYSCTL.laser_difference;
	payload[5] = breakbeam_diff; // Break beam reading LSB
	payload[6] = breakbeam_diff >> 8; // Break beam reading MSB

	adc_value = IO_SYSCTL.thermistor_voltage;
	payload[7] = adc_value; // Board temperature LSB
	payload[8] = adc_value >> 8; // Board temperature MSB

	uint8_t flags = 0;
	if (breakbeam_diff < BREAKBEAM_DIFF_THRESHOLD) {
		flags |= 0x01;
	}
	io_chicker_csr_t chicker_csr = IO_CHICKER.csr;
	if (chicker_csr.charge_done) {
		flags |= 0x02;
	}
	if (chicker_csr.charge_timeout) {
		flags |= 0x04;
	}
	if (IO_SYSCTL.csr.breakout_present) {
		flags |= 0x08;
	}
	if (chicker_csr.present) {
		flags |= 0x10;
	}
	if (!IO_SYSCTL.csr.hardware_interlock) {
		flags |= 0x40;
	}
	payload[9] = flags; // Flags

	flags = 0;
	memcpy(radio_reporting_motor_sensor_failures, radio_pending_motor_sensor_failures, sizeof(radio_reporting_motor_sensor_failures));
	memset(radio_pending_motor_sensor_failures, 0, sizeof(radio_pending_motor_sensor_failures));
	for (unsigned int wheel = 0; wheel < 4; ++wheel) {
		flags |= (radio_reporting_motor_sensor_failures[wheel] & 3) << (wheel * 2);
	}
	payload[10] = flags; // Wheel motor Hall sensors stuck

	flags = radio_reporting_motor_sensor_failures[4] & 3;
	for (unsigned int wheel = 0; wheel < 4; ++wheel) {
		flags |= (radio_reporting_motor_sensor_failures[wheel] & 4) << wheel;
	}
	payload[11] = flags; // Dribbler Hall sensors stuck and optical encoders not commutating

	if (log_overflow_feedback_report_pending) {
		payload[12] = (LOG_STATE_OVERFLOW << 4) | SD_STATUS_OK;
		log_overflow_feedback_report_pending = false;
	} else {
		payload[12] = (log_state() << 4) | sd_status();
	}

	payload[13] = dribbler_speed;
	payload[14] = dribbler_speed >> 8;

	payload[15] = wheels_hot | (dribbler_hot ? 1 << 4 : 0);
#undef payload

	radio_tx_packet_reliable = false;
	radio_tx_packet_prepared = true;
	radio_has_prepared_feedback = true;
}

static void prepare_autokick_packet(void) {
	prepare_mrf_mhr(1);

#define payload (&mrf_tx_buffer[11])
	payload[0] = 0x01; // Autokick fired
#undef payload

	radio_tx_packet_reliable = true;
	radio_tx_packet_prepared = true;
}

static void prepare_breakbeam_packet(bool status) {
	prepare_mrf_mhr(1);

#define payload (&mrf_tx_buffer[11])
	payload[0] = status ? 0x04 : 0x05;
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
				// Broadcast frame must contain drive packet, which must be 65 bytes long
				if (frame_length == HEADER_LENGTH + 65 + FOOTER_LENGTH) {
					// Construct the individual 16-bit words sent from the host.
					last_drive_packet_time = IO_SYSCTL.tsc;
					const uint8_t offset = 1 + HEADER_LENGTH + 8 * robot_index;
					uint16_t words[4];
					for (uint8_t i = 0; i < 4; ++i) {
						words[i] = mrf_rx_buffer[offset + i * 2 + 1];
						words[i] <<= 8;
						words[i] |= mrf_rx_buffer[offset + i * 2];
					}

					// Pull out the serial number at the end.
					uint8_t drive_data_serial = mrf_rx_buffer[1 + HEADER_LENGTH + 64];

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
							if (drive_data_serial != last_drive_data_serial) {
								control_process_new_setpoints(new_setpoints);
							}
						} else {
							memcpy(wheels_setpoints.wheels, new_setpoints, sizeof(new_setpoints));
						}
					}
					last_drive_data_serial = drive_data_serial;

					// Set new dribbler mode.
					// Delay sending new mode to motors until after evaluating drivetrain power to avoid possible latchup in motor drivers if drivetrain is unpowered.
					dribbler_enabled = !!(words[0] & (1 << 12));
					dribbler_fast = !(words[0] & (1 << 11));

					// Set new chicker mode.
					{
						io_chicker_csr_t csr = IO_CHICKER.csr;
						switch ((words[1] >> 14) & 3) {
							case 0b00:
								csr.charge = false;
								csr.discharge = false;
								break;
							case 0b01:
								csr.charge = false;
								csr.discharge = true;
								break;
							case 0b10:
								csr.charge = true;
								csr.discharge = false;
								break;
						}
						IO_CHICKER.csr = csr;
					}

					// Turn drivetrain power on or off if needed.
					if (drivetrain_power_forced_on || wheels_mode != WHEELS_MODE_MANUAL_COMMUTATION || dribbler_enabled) {
						IO_SYSCTL.csr.motor_power = 1;
					} else {
						IO_SYSCTL.csr.motor_power = 0;
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
							IO_CHICKER.pulse_width = width;
							{
								if (which) {
									IO_CHICKER.csr.chip = 1;
								} else {
									IO_CHICKER.csr.kick = 1;
								}
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
						}
						break;

					case 0x08: // Reboot
						if (frame_length == HEADER_LENGTH + 1 + FOOTER_LENGTH) {
							static char MESSAGE[] = "Rebooting at user request.\n";
							syscall_debug_puts(MESSAGE);
							shutdown_sequence(true);
						}
						break;

					case 0x09: // Force on motor power
						drivetrain_power_forced_on = true;
						break;

					case 0x0B: // Set bootup radio parameters
						if (frame_length == HEADER_LENGTH + 5 + FOOTER_LENGTH) {
							// Erase and rewrite the sector
							flash_write_params(&mrf_rx_buffer[MESSAGE_PAYLOAD_ADDR], 4);

							static char MESSAGE[] = "Parameters rewritten.\n";
							syscall_debug_puts(MESSAGE);
						}
						break;

					case 0x0C: // Shut down
						{
							static char MESSAGE[] = "Shutting down at operator request.\n";
							syscall_debug_puts(MESSAGE);
							shutdown_sequence(false);
						}

					case 0x0D: // Manually commute motors
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
	// Update IIR filter on battery voltage and check for low battery.
	battery_average = (battery_average * (BATTERY_AVERAGE_FACTOR - 1) + IO_SYSCTL.battery_voltage * BATTERY_VOLTS_PER_LSB) / BATTERY_AVERAGE_FACTOR;
	if (battery_average < BATTERY_LOW_THRESHOLD && IO_SYSCTL.csr.software_interlock) {
		static char MESSAGE[] = "Shutting down due to low battery.\n";
		syscall_debug_puts(MESSAGE);
		shutdown_sequence(false);
	}

	// Scan for motor sensor failures and record them both locally and in the pending feedback report buffer.
	uint8_t sensor_failures[5];
	for (unsigned int i = 0; i < 5; ++i) {
		uint8_t status = motor_sensor_failed(i);
		sensor_failures[i] = status;
		radio_pending_motor_sensor_failures[i] |= status;
	}

	// Compute instantaneous battery voltage in actual volts.
	float battery_volts = IO_SYSCTL.battery_voltage * BATTERY_VOLTS_PER_LSB;

	// Run the wheels.
	wheels_tick(battery_volts, sensor_failures);

	// Run the dribbler.
	dribbler_tick(battery_volts);

	// Write a log record if possible.
	log_record_t *rec = log_alloc();
	if (rec) {
		rec->magic = LOG_MAGIC_TICK;

		rec->tick.breakbeam_diff = IO_SYSCTL.laser_difference;
		rec->tick.battery_voltage = IO_SYSCTL.battery_voltage;
		rec->tick.capacitor_voltage = IO_CHICKER.capacitor_voltage;
		rec->tick.board_temperature = IO_SYSCTL.thermistor_voltage;

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
		for (uint8_t i = 0; i < sizeof(rec->tick.wheels_temperatures) / sizeof(*rec->tick.wheels_temperatures); ++i) {
			rec->tick.wheels_temperatures[i] = (uint8_t) (wheels_energy[i] / WHEEL_THERMAL_CAPACITANCE + WHEEL_THERMAL_AMBIENT);
		}

		rec->tick.dribbler_temperature = (uint8_t) (dribbler_winding_energy / DRIBBLER_THERMAL_CAPACITANCE_WINDING + DRIBBLER_THERMAL_AMBIENT);

		{
			uint8_t acc = 0;
			for (unsigned int wheel = 0; wheel < 4; ++wheel) {
				acc |= (sensor_failures[wheel] >> 2) << wheel;
			}
			rec->tick.encoders_failed = acc;
		}

		{
			uint8_t acc = 0;
			for (unsigned int wheel = 0; wheel < 4; ++wheel) {
				acc |= (sensor_failures[wheel] & 3) << (2 * wheel);
			}
			rec->tick.wheels_hall_sensors_failed = acc;
		}

		rec->tick.dribbler_hall_sensors_failed = sensor_failures[4];

		rec->tick.dribbler_speed = dribbler_speed;

		rec->tick.cpu_used_since_last_tick = cpu_usage;
		cpu_usage = 0;

		log_commit();
	} else if (log_state() == LOG_STATE_OVERFLOW) {
		log_overflow_feedback_report_pending = true;
	}
}

static void lookup_vid_did(unsigned int vid, unsigned int did, const char **vendor, const char **device) {
	switch (vid) {
		case 0x01:
			*vendor = "Gaisler Research";
			switch (did) {
				case 3: *device = "LEON3"; break;
				case 6: *device = "APB Bridge"; break;
				case 14: *device = "AHB RAM"; break;
				default: *device = "Unknown"; break;
			}
			break;

		case 0xFF:
			*vendor = "Thunderbots";
			switch (did) {
				case 1: *device = "AHB ROM"; break;
				case 2: *device = "UART debug port"; break;
				case 3: *device = "SPI Flash"; break;
				case 4: *device = "System controller"; break;
				case 5: *device = "MRF24J40 interface"; break;
				case 6: *device = "Motor controller"; break;
				case 7: *device = "Chicker controller"; break;
				case 8: *device = "Secure Digital host controller"; break;
				default: *device = "Unknown"; break;
			}
			break;

		default:
			*vendor = "Unknown";
			*device = "Unknown";
			break;
	}
}

static void print_pnp_devid(unsigned int i, const io_pnp_devid_t *devid) {
	if (devid->vendor_id) {
		char buffer[128];
		if (devid->config_version != 0) {
			siprintf(buffer, "[%u]: Unknown config record version %u\n", i, (unsigned int) devid->config_version);
		} else {
			const char *vendor, *device;
			lookup_vid_did(devid->vendor_id, devid->device_id, &vendor, &device);
			siprintf(buffer, "[%u]: Vendor %u (%s), device %u (%s), version %u, IRQ %u\n", i, (unsigned int) devid->vendor_id, vendor, (unsigned int) devid->device_id, device, (unsigned int) devid->version, (unsigned int) devid->irq);
		}
		syscall_debug_puts(buffer);
	}
}

static void print_pnp_bar(unsigned int address_base, const io_pnp_bar_t *bar) {
	const char *type;
	unsigned int address_shift;
	switch (bar->type) {
		case 1: type = "APB I/O"; address_shift = 8; break;
		case 2: type = "AHB memory"; address_shift = 20; break;
		case 3: type = "AHB I/O"; address_shift = 8; break;
		default: type = 0; address_shift = 0; break;
	}
	if (type) {
		char buffer[64];
		unsigned int bar_base = address_base + (bar->address << address_shift);
		unsigned int bar_top = address_base + (((bar->address | ~bar->mask) + 1) << address_shift) - 1;
		siprintf(buffer, "\t%s BAR: %p–%p [%c%c]\n", type, (void *) bar_base, (void *) bar_top, bar->prefetchable ? 'P' : 'p', bar->cacheable ? 'C' : 'c');
		syscall_debug_puts(buffer);
	}
}

int main(void) {
	// Print an application startup message.
	{
		static char MESSAGE[] = "APP INIT\n";
		syscall_debug_puts(MESSAGE);
	}

	// Print the plug-and-play data on the debug port.
	{
		static char MESSAGE[] = "== AHB masters ==\n";
		syscall_debug_puts(MESSAGE);
		for (unsigned int i = 0; i < 16; ++i) {
			print_pnp_devid(i, &IO_AHB_PNP_DATA.masters[i].devid);
			for (unsigned int j = 0; j < 4; ++j) {
				print_pnp_bar(0, &IO_AHB_PNP_DATA.masters[i].bars[j]);
			}
		}
	}
	{
		static char MESSAGE[] = "== AHB slaves ==\n";
		syscall_debug_puts(MESSAGE);
		for (unsigned int i = 0; i < 16; ++i) {
			print_pnp_devid(i, &IO_AHB_PNP_DATA.slaves[i].devid);
			for (unsigned int j = 0; j < 4; ++j) {
				print_pnp_bar(0, &IO_AHB_PNP_DATA.slaves[i].bars[j]);
			}
		}
	}
	{
		static char MESSAGE[] = "== APB slaves ==\n";
		syscall_debug_puts(MESSAGE);
		for (unsigned int i = 0; i < 16; ++i) {
			print_pnp_devid(i, &IO_APB_PNP_DATA[i].devid);
			print_pnp_bar(IO_APB_BASE, &IO_APB_PNP_DATA[i].bar);
		}
	}

	// Print the device DNA on the debug port.
	{
		uint64_t device_dna = device_dna_read();
		char buffer[32];
		siprintf(buffer, "\n\nDevice DNA: %014" PRIX64 "\n", device_dna);
		syscall_debug_puts(buffer);
	}

	// Read the parameters from the end of the SPI flash.
	{
		channel = flash_params_block[0];
		robot_index = flash_params_block[1];
		pan = (flash_params_block[2] << 8) | flash_params_block[3];
		if (0x0B <= channel && channel <= 0x1A && robot_index <= 7 && pan != 0xFFFF) {
			// Parameters OK.
		} else {
			// Parameters invalid.
			channel = DEFAULT_CHANNEL;
			robot_index = DEFAULT_INDEX;
			pan = DEFAULT_PAN;
		}
		char buffer[64];
		siprintf(buffer, "Radio: channel %" PRIu8 ", index %" PRIu8 ", PAN 0x%04" PRIX16 "\n", channel, robot_index, pan);
		syscall_debug_puts(buffer);
	}

	// Initialize the SD card and the logging layer.
	if (sd_init()) {
		log_init();
	}

	// Initialize the radio.
	mrf_init(channel, false, pan, robot_index, UINT64_C(0xec89d61e8ffd409b));

	// Turn on the radio LED.
	IO_SYSCTL.csr.radio_led = 1;

	// Initialize the battery average to the current level.
	battery_average = IO_SYSCTL.battery_voltage * BATTERY_VOLTS_PER_LSB;

	// Initialize the motors.
	motor_init();

	// Initialize a tick count.
	unsigned int last_control_loop_time = radio_last_blink_time = IO_SYSCTL.tsc;
	for(;;) {
		// Check if an autokick needs to fire.
		// Autokick should fire if it is armed, if the breakbeam is interrupted, and if, for autochip, the dribbler is slow.
		if (autokick_armed && IO_SYSCTL.laser_difference < BREAKBEAM_DIFF_THRESHOLD && (!autokick_device || dribbler_speed < 4)) {
			IO_CHICKER.pulse_width = autokick_pulse_width;
			if (autokick_device) {
				IO_CHICKER.csr.chip = 1;
			} else {
				IO_CHICKER.csr.kick = 1;
			}
			autokick_armed = false;
			autokick_fired_pending = true;
		}

		// Check if a tick has passed; if so, iterate the control loop, update average battery level, and log a tick record.
		{
			unsigned int now = IO_SYSCTL.tsc;
			if (now - last_control_loop_time >= (F_CPU / CONTROL_LOOP_HZ)) {
				last_control_loop_time = now;
				handle_tick();
				cpu_usage += IO_SYSCTL.tsc - now;
			}
		}

		// Check if a radio packet has been received.
		if (mrf_rx_poll()) {
			unsigned int start = radio_last_blink_time = IO_SYSCTL.tsc;
			IO_SYSCTL.csr.radio_led = !IO_SYSCTL.csr.radio_led;
			handle_radio_receive();
			cpu_usage += IO_SYSCTL.tsc - start;
		}

		// Check if it’s time to turn the radio LED on after it was blinking.
		if (IO_SYSCTL.tsc - radio_last_blink_time > F_CPU / 5) {
			IO_SYSCTL.csr.radio_led = 1;
			radio_last_blink_time = IO_SYSCTL.tsc;
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
		// 4. For feedback packets, when the path becomes free again, we check the transmission status to determine whether we must retain some error reports for next time.

		// Check if we should assemble a packet now.
		if (mrf_tx_buffer_free() && !radio_tx_packet_prepared) {
			bool breakbeam = IO_SYSCTL.laser_difference < BREAKBEAM_DIFF_THRESHOLD;
			if (breakbeam != last_breakbeam_report) {
				unsigned int start = IO_SYSCTL.tsc;
				prepare_breakbeam_packet(breakbeam);
				last_breakbeam_report = breakbeam;
				cpu_usage += IO_SYSCTL.tsc - start;
			} else if (feedback_pending) {
				unsigned int start = IO_SYSCTL.tsc;
				prepare_feedback_packet();
				feedback_pending = false;
				cpu_usage += IO_SYSCTL.tsc - start;
			} else if (autokick_fired_pending) {
				unsigned int start = IO_SYSCTL.tsc;
				prepare_autokick_packet();
				autokick_fired_pending = false;
				cpu_usage += IO_SYSCTL.tsc - start;
			}
		}

		// Check if the transmit path is free, in which case we are able to (1) check the status of a prior transmission, and (2) start a new transmission.
		if (mrf_tx_path_free()) {
			// Check whether the last packet we were sending was a feedback packet.
			if (radio_was_sending_feedback) {
				// A feedback packet was sent.
				// Check whether it was delivered successfully.
				if (!mrf_tx_successful()) {
					// Delivery failed.
					// Accumulate the errors we were trying to report back into the pending set so they will be reported in the next feedback.
					for (unsigned int i = 0; i < sizeof(radio_pending_motor_sensor_failures) / sizeof(*radio_pending_motor_sensor_failures); ++i) {
						radio_pending_motor_sensor_failures[i] |= radio_reporting_motor_sensor_failures[i];
					}
				}
				radio_was_sending_feedback = false;
			}

			// Check if a packet has been prepared and the MRF transmit path is ready to transmit it.
			if (radio_tx_packet_prepared) {
				unsigned int start = IO_SYSCTL.tsc;
				mrf_tx_start(radio_tx_packet_reliable);
				radio_tx_packet_prepared = false;
				radio_was_sending_feedback = radio_has_prepared_feedback;
				radio_has_prepared_feedback = false;
				cpu_usage += IO_SYSCTL.tsc - start;
			}
		}

		// Update the LEDs.
		if (led_mode <= 4) {
			IO_SYSCTL.csr.test_leds = IO_MOTOR(led_mode).sensors & 7;
		} else if (led_mode <= 8) {
			IO_SYSCTL.csr.test_leds = IO_MOTOR(led_mode - 5).sensors >> 3;
		} else if (led_mode == 0x20) {
			uint8_t flags = 0;
			if (IO_SYSCTL.laser_difference < BREAKBEAM_DIFF_THRESHOLD) {
				flags |= 0x01;
			}
			if (autokick_armed) {
				flags |= 0x04;
			}
			IO_SYSCTL.csr.test_leds = flags;
		} else if (led_mode == 0x21) {
			IO_SYSCTL.csr.test_leds = 7;
		} else {
			IO_SYSCTL.csr.test_leds = 0;
		}

		// Check if more than a second has passed since we last received a fresh drive packet.
		if (IO_SYSCTL.tsc - last_drive_packet_time > F_CPU) {
			// Time out and stop driving.
			unsigned int start = IO_SYSCTL.tsc;
			memset(&wheels_setpoints, 0, sizeof(wheels_setpoints));
			wheels_mode = WHEELS_MODE_MANUAL_COMMUTATION;
			dribbler_enabled = false;
			IO_CHICKER.csr.charge = 0;
			cpu_usage += IO_SYSCTL.tsc - start;
		}

		// Check if the board temperature is over 100°C and interlocks are enabled.
		if (IO_SYSCTL.thermistor_voltage < HIGH_TEMPERATURE_THRESHOLD && IO_SYSCTL.csr.software_interlock) {
			// Turn off all the motors.
			unsigned int start = IO_SYSCTL.tsc;
			memset(&wheels_setpoints, 0, sizeof(wheels_setpoints));
			wheels_mode = WHEELS_MODE_MANUAL_COMMUTATION;
			dribbler_enabled = false;
			motor_scram();
			cpu_usage += IO_SYSCTL.tsc - start;
		}

		// Do not dribble if autochip is armed; this makes the chipper more effective.
		if (autokick_armed && autokick_device) {
			dribbler_enabled = false;
		}
	}
}

