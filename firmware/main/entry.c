#include "adc.h"
#include "chicker.h"
#include "debug.h"
#include "device_dna.h"
#include "flash.h"
#include "io.h"
#include "led.h"
#include "motor.h"
#include "mrf.h"
#include "power.h"
#include "sleep.h"
#include "wheels.h"
#include <inttypes.h>
#include <stdio.h>

#define DEFAULT_CHANNEL 20
#define DEFAULT_PAN 0x1846
#define DEFAULT_INDEX 0

#define BREAKBEAM_THRESHOLD 300

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
static bool erasing_flash = false;
static uint8_t flash_page_buffer[256];
static uint32_t region_sum;
static bool region_sum_pending = false;
static uint16_t autokick_pulse_width;
static uint8_t autokick_device;
static bool autokick_armed = false;
static bool autokick_fired_pending = false;

void prepare_mrf_mhr(uint8_t payload_length) {
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 0, 9); // Header length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 1, 9 + payload_length); // Frame length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 2, 0b01100001); // Frame control LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 3, 0b10001000); // Frame control MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 4, tx_seqnum++); // Sequence number
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 5, pan & 0xFF); // Destination PAN ID LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 6, pan >> 8); // Destination PAN ID MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 7, 0x00); // Destination address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 8, 0x01); // Destination address MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 9, index); // Source address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 10, 0); // Source address MSB
}

static void send_feedback_packet(void) {
	prepare_mrf_mhr(8);

	mrf_write_long(MRF_REG_LONG_TXNFIFO + 11, 0x00); // General robot status update
	uint16_t adc_value = read_main_adc(BATT_VOLT);
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 12, adc_value); // Battery voltage LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 13, adc_value >> 8); // Battery voltage MSB
	adc_value = read_main_adc(CHICKER);
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 14, adc_value); // Capacitor voltage LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 15, adc_value >> 8); // Capacitor voltage MSB
	uint16_t breakbeam_value = read_main_adc(BREAKBEAM);
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 16, breakbeam_value); // Break beam reading LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 17, breakbeam_value >> 8); // Break beam reading MSB
	uint8_t flags = 0;
	if (breakbeam_value > BREAKBEAM_THRESHOLD) {
		flags |= 0x01;
	}
	if (is_charged()) {
		flags |= 0x02;
	}
	if (is_charge_timeout()) {
		flags |= 0x04;
	}
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 18, flags); // Flags

	mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);

	transmit_busy = true;
	feedback_pending = false;
	transmission_reliable = false;
}

static void handle_radio_receive(void) {
	// Blink a light
	radio_led_blink();

	mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception
	uint8_t frame_length = mrf_read_long(MRF_REG_LONG_RXFIFO + 0);
	uint16_t frame_control = mrf_read_long(MRF_REG_LONG_RXFIFO + 1) | (mrf_read_long(MRF_REG_LONG_RXFIFO + 2) << 8);
	// Sanity-check the frame control word
	if (((frame_control >> 0) & 7) == 1 /* Data packet */ && ((frame_control >> 3) & 1) == 0 /* No security */ && ((frame_control >> 6) & 1) == 1 /* Intra-PAN */ && ((frame_control >> 10) & 3) == 2 /* 16-bit destination address */ && ((frame_control >> 14) & 3) == 2 /* 16-bit source address */) {
		// Read out and check the source address and sequence number
		uint16_t source_address = mrf_read_long(MRF_REG_LONG_RXFIFO + 8) | (mrf_read_long(MRF_REG_LONG_RXFIFO + 9) << 8);
		uint8_t sequence_number = mrf_read_long(MRF_REG_LONG_RXFIFO + 3);
		if (source_address == 0x0100 && sequence_number != rx_seqnum) {
			// Update sequence number
			rx_seqnum = sequence_number;

			// Handle packet
			uint16_t dest_address = mrf_read_long(MRF_REG_LONG_RXFIFO + 6) | (mrf_read_long(MRF_REG_LONG_RXFIFO + 7) << 8);
			static const uint8_t HEADER_LENGTH = 2 /* Frame control */ + 1 /* Seq# */ + 2 /* Dest PAN */ + 2 /* Dest */ + 2 /* Src */;
			static const uint8_t FOOTER_LENGTH = 2;
			if (dest_address == 0xFFFF) {
				// Broadcast frame must contain drive packet, which must be 64 bytes long
				if (frame_length == HEADER_LENGTH + 64 + FOOTER_LENGTH) {
					const uint8_t offset = 1 + HEADER_LENGTH + 8 * index;
					uint16_t words[4];
					for (uint8_t i = 0; i < 4; ++i) {
						words[i] = mrf_read_long(MRF_REG_LONG_RXFIFO + offset + i * 2 + 1);
						words[i] <<= 8;
						words[i] |= mrf_read_long(MRF_REG_LONG_RXFIFO + offset + i * 2);
						wheel_setpoint[i] = words[i] & 0x3FF;
						if (words[i] & 0x400) {
							wheel_setpoint[i] = -wheel_setpoint[i];
						}
					}
					feedback_pending = !!(words[0] & 0x8000);
					switch ((words[0] >> 13) & 0b11) {
						case 0b00: wheel_mode = WHEEL_MODE_COAST; break;
						case 0b01: wheel_mode = WHEEL_MODE_BRAKE; break;
						case 0b10: wheel_mode = WHEEL_MODE_OPEN_LOOP; break;
						case 0b11: wheel_mode = WHEEL_MODE_CLOSED_LOOP; break;
					}
					if (wheel_mode != WHEEL_MODE_COAST && wheel_mode != WHEEL_MODE_BRAKE) {
						power_enable_motors();
					}
					if (words[0] & (1 << 12)) {
						power_enable_motors();
						set_dribbler(FORWARD, 180);
					} else {
						set_dribbler(FLOAT, 0);
					}
					switch ((words[1] >> 14) & 3) {
						case 0b00:
							set_charge_mode(false);
							set_discharge_mode(false);
							break;
						case 0b01:
							power_enable_chicker();
							set_charge_mode(false);
							set_discharge_mode(true);
							break;
						case 0b10:
							power_enable_chicker();
							set_discharge_mode(false);
							set_charge_mode(true);
							break;
					}
				}
			} else if (frame_length >= HEADER_LENGTH + 1 + FOOTER_LENGTH) {
				// Non-broadcast frame contains a message specifically for this robot
				static const uint16_t MESSAGE_PURPOSE_ADDR = MRF_REG_LONG_RXFIFO + 1 + HEADER_LENGTH;
				static const uint16_t MESSAGE_PAYLOAD_ADDR = MESSAGE_PURPOSE_ADDR + 1;
				switch (mrf_read_long(MESSAGE_PURPOSE_ADDR)) {
					case 0x00: // Fire kicker immediately
						if (frame_length == HEADER_LENGTH + 4 + FOOTER_LENGTH) {
							uint8_t which = mrf_read_long(MESSAGE_PAYLOAD_ADDR);
							uint16_t width = mrf_read_long(MESSAGE_PAYLOAD_ADDR + 2);
							width <<= 8;
							width |= mrf_read_long(MESSAGE_PAYLOAD_ADDR + 1);
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
							autokick_device = mrf_read_long(MESSAGE_PAYLOAD_ADDR);
							autokick_pulse_width = mrf_read_long(MESSAGE_PAYLOAD_ADDR + 2);
							autokick_pulse_width <<= 8;
							autokick_pulse_width |= mrf_read_long(MESSAGE_PAYLOAD_ADDR + 1);
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
							led_mode = mrf_read_long(MESSAGE_PAYLOAD_ADDR);
							if (led_mode <= 0x1F) {
								outb(LED_CTL, (inb(LED_CTL) & 0x80) | led_mode);
							} else if (led_mode == 0x21) {
								set_test_leds(USER_MODE, 7);
							} else {
								set_test_leds(USER_MODE, 0);
							}
						}
						break;

					case 0x04: // Erase SPI flash
						if (frame_length == HEADER_LENGTH + 1 + FOOTER_LENGTH) {
							flash_start_chip_erase();
							erasing_flash = true;
						}
						break;

					case 0x05: // Fill page buffer
						if (frame_length >= HEADER_LENGTH + 3 + FOOTER_LENGTH) {
							uint8_t offset = mrf_read_long(MESSAGE_PAYLOAD_ADDR);
							if (((uint16_t) offset) + (frame_length - HEADER_LENGTH - 2 - FOOTER_LENGTH) <= 256) {
								for (uint8_t i = 0; i < frame_length - HEADER_LENGTH - 2 - FOOTER_LENGTH; ++i) {
									flash_page_buffer[offset + i] = mrf_read_long(MESSAGE_PAYLOAD_ADDR + 1 + i);
								}
							}
						}
						break;

					case 0x06: // Write page buffer to SPI flash
						if (frame_length == HEADER_LENGTH + 3 + FOOTER_LENGTH) {
							uint16_t page = mrf_read_long(MESSAGE_PAYLOAD_ADDR) | (mrf_read_long(MESSAGE_PAYLOAD_ADDR + 1) << 8);
							flash_page_program(page, flash_page_buffer, 0);
						}
						break;

					case 0x07: // Sum region of SPI flash
						if (frame_length == HEADER_LENGTH + 7 + FOOTER_LENGTH) {
							uint32_t address = mrf_read_long(MESSAGE_PAYLOAD_ADDR) | (((uint32_t) mrf_read_long(MESSAGE_PAYLOAD_ADDR + 1)) << 8) | (((uint32_t) mrf_read_long(MESSAGE_PAYLOAD_ADDR + 2)) << 16);
							uint32_t length = mrf_read_long(MESSAGE_PAYLOAD_ADDR + 3) | (((uint32_t) mrf_read_long(MESSAGE_PAYLOAD_ADDR + 4)) << 8) | (((uint32_t) mrf_read_long(MESSAGE_PAYLOAD_ADDR + 5)) << 16);
							region_sum = flash_sum(address, length);
							region_sum_pending = true;
						}
						break;

					case 0x08: // Reboot
						if (frame_length == HEADER_LENGTH + 1 + FOOTER_LENGTH) {
							power_reboot();
						}
						break;

					case 0x09: // Force on motor power
						power_enable_motors();
						break;

					case 0x0A: // Force on chicker power
						power_enable_chicker();
						break;

					case 0x0B: // Set bootup radio parameters
						if (frame_length == HEADER_LENGTH + 5 + FOOTER_LENGTH) {
							// Extract the new data from the radio
							uint8_t buffer[4];
							for (uint8_t i = 0; i < sizeof(buffer); ++i) {
								buffer[i] = mrf_read_long(MESSAGE_PAYLOAD_ADDR + i);
							}

							// Erase and rewrite the sector
							flash_erase_sector(SPI_FLASH_PARAMETERS_ADDRESS);
							flash_page_program(SPI_FLASH_PARAMETERS_ADDRESS >> 8, buffer, sizeof(buffer));

							puts("Parameters rewritten.");
						}
						break;

					case 0x0C: // Shut down
						{
							puts("Shutting down.");
							set_charge_mode(false);
							set_discharge_mode(true);
							motor_scram();
							sleep_1s();
							sleep_1s();
							sleep_1s();
							outb(POWER_CTL, 0x01);
							sleep_1s();
							outb(POWER_CTL, 0x00);
							for (;;);
						}
				}
			}
		}
	}
	mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
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

	// Initialize the radio
	mrf_init();
	sleep_short();
	mrf_release_reset();
	sleep_short();
	mrf_common_init(channel, false, pan, UINT64_C(0xec89d61e8ffd409b));
	while (inb(MRF_CTL) & 0x10);
	mrf_write_short(MRF_REG_SHORT_SADRH, 0);
	mrf_write_short(MRF_REG_SHORT_SADRL, index);
	mrf_analogue_txrx();
	mrf_write_short(MRF_REG_SHORT_INTCON, 0b11110110);

	// Turn on the radio LED
	radio_led_ctl(true);

	// Turn on the break beam laser
	power_enable_laser();

	// Initialize a tick count
	uint8_t old_ticks = inb(TICKS);
	for(;;) {
		// Check if an autokick needs to fire
		if (autokick_armed && read_main_adc(BREAKBEAM) > BREAKBEAM_THRESHOLD) {
			set_chick_pulse(autokick_pulse_width);
			if (autokick_device) {
				fire_chipper();
			} else {
				fire_kicker();
			}
			autokick_armed = false;
			autokick_fired_pending = true;
		}

		// Check if a tick has passed; if so, iterate the control loop
		if (inb(TICKS) != old_ticks) {
			wheels_tick();
			++old_ticks;
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
			if (intstat & (1 << 3)) {
				// Packet received
				handle_radio_receive();
			}
		}

		// Check if we should send a feedback packet now
		if (feedback_pending && !transmit_busy) {
			send_feedback_packet();
		}

		// Update the LEDs
		if (led_mode == 0x20) {
			uint8_t flags = 0;
			if (read_main_adc(BREAKBEAM) > BREAKBEAM_THRESHOLD) {
				flags |= 0x01;
			}
			if (autokick_armed) {
				flags |= 0x04;
			}
			set_test_leds(USER_MODE, flags);
		}

		// Check if an in-progress Flash erase operation has now finished
		if (erasing_flash && !flash_is_busy()) {
			if (!transmit_busy) {
				// Send notification to host
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
				prepare_mrf_mhr(1);
				mrf_write_long(MRF_REG_LONG_TXNFIFO + 11, 0x02); // SPI flash erase finished

				mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);

				transmit_busy = true;
				erasing_flash = false;
				transmission_reliable = true;
			}
		}

		// Check if a flash region sum operation finished and the sum value needs transmitting
		if (region_sum_pending && !transmit_busy) {
			// Send notification to host
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
			prepare_mrf_mhr(5);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11, 0x03); // SPI region sum
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 12, region_sum);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 13, region_sum >> 8);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 14, region_sum >> 16);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 15, region_sum >> 24);

			mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);

			transmit_busy = true;
			region_sum_pending = false;
			transmission_reliable = true;
		}

		// Check if the autokick system fired and the report needs transmitting
		if (autokick_fired_pending && !transmit_busy) {
			// Send notification to host
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
			prepare_mrf_mhr(1);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11, 0x01); // Autokick fired

			mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);

			transmit_busy = true;
			autokick_fired_pending = false;
			transmission_reliable = true;
		}
	}
}

