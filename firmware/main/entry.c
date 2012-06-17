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

#define CHANNEL 0x16
#define PAN 0x1846
#define INDEX 0

#define BREAKBEAM_THRESHOLD 800

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

static bool transmit_busy = false, feedback_pending = false;
static uint8_t tx_seqnum = 0;
static uint16_t rx_seqnum = 0xFFFF;
static uint8_t led_mode = 0x20;
static bool erasing_flash = false;
static uint8_t flash_page_buffer[256];
static uint32_t region_sum;
static bool region_sum_pending = false;

static void send_feedback_packet(void) {
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 0, 9); // Header length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 1, 9 + 10); // Frame length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 2, 0b01100001); // Frame control LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 3, 0b10001000); // Frame control MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 4, tx_seqnum++); // Sequence number
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 5, PAN & 0xFF); // Destination PAN ID LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 6, PAN >> 8); // Destination PAN ID MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 7, 0x00); // Destination address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 8, 0x01); // Destination address MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 9, INDEX); // Source address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 10, 0); // Source address MSB

	mrf_write_long(MRF_REG_LONG_TXNFIFO + 11, 0x00); // General robot status update
	uint16_t adc_value = read_main_adc(BATT_VOLT);
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 12, adc_value); // Battery voltage LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 13, adc_value >> 8); // Battery voltage MSB
	adc_value = read_chicker_adc();
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 14, adc_value); // Capacitor voltage LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 15, adc_value >> 8); // Capacitor voltage MSB
	uint16_t breakbeam_value = read_main_adc(BREAKBEAM);
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 16, breakbeam_value); // Break beam reading LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 17, breakbeam_value >> 8); // Break beam reading MSB
	adc_value = read_main_adc(THERMISTOR);
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 18, adc_value); // Thermistor reading LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 19, adc_value >> 8); // Thermistor reading MSB
	uint8_t flags = 0;
	if (breakbeam_value < BREAKBEAM_THRESHOLD) {
		flags |= 0x01;
	}
	if (is_charged()) {
		flags |= 0x02;
	}
	if (is_charge_timeout()) {
		flags |= 0x04;
	}
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 20, flags); // Flags

	mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);

	transmit_busy = true;
	feedback_pending = false;
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
					static const uint8_t offset = 1 + HEADER_LENGTH + 8 * INDEX;
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
					outb(BREAK_BEAM_CTL, 1);
				}
			} else if (frame_length >= HEADER_LENGTH + 1 + FOOTER_LENGTH) {
				// Non-broadcast frame contains a message specifically for this robot
				static const uint16_t MESSAGE_PURPOSE_ADDR = MRF_REG_LONG_RXFIFO + 1 + HEADER_LENGTH;
				static const uint16_t MESSAGE_PAYLOAD_ADDR = MESSAGE_PURPOSE_ADDR + 1;
				switch (mrf_read_long(MESSAGE_PURPOSE_ADDR)) {
					case 0x00: // Fire kicker immediately
						if (frame_length == HEADER_LENGTH + 4 + FOOTER_LENGTH) {
							set_test_leds(USER_MODE, 7);
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
							flash_assert_cs();
							flash_tx(0x06);
							flash_deassert_cs();

							flash_assert_cs();
							flash_tx(0xC7);
							flash_deassert_cs();

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

							flash_assert_cs();
							flash_tx(0x06);
							flash_deassert_cs();

							flash_assert_cs();
							flash_tx(0x02);
							flash_tx(page >> 8);
							flash_tx(page);
							flash_tx(0);
							uint8_t i = 0;
							do {
								flash_tx(flash_page_buffer[i]);
							} while (++i);
							flash_deassert_cs();

							asm volatile("nop");
							asm volatile("nop");
							asm volatile("nop");
							asm volatile("nop");

							uint8_t status_register;
							flash_assert_cs();
							flash_tx(0x05);
							do {
								status_register = flash_txrx(0x00);
							} while (status_register & 0x01);
							flash_deassert_cs();

							flash_assert_cs();
							flash_tx(0x03);
							flash_tx(page >> 8);
							flash_tx(page);
							flash_tx(0);
							i = 0;
							do {
								if (flash_txrx(0x00) != flash_page_buffer[i]) {
									set_test_leds(USER_MODE, 7);
									for (;;);
								}
							} while (++i);
							flash_deassert_cs();
						}
						break;

					case 0x07: // Sum region of SPI flash
						if (frame_length == HEADER_LENGTH + 7 + FOOTER_LENGTH) {
							uint32_t address = mrf_read_long(MESSAGE_PAYLOAD_ADDR) | (((uint32_t) mrf_read_long(MESSAGE_PAYLOAD_ADDR + 1)) << 8) | (((uint32_t) mrf_read_long(MESSAGE_PAYLOAD_ADDR + 2)) << 16);
							uint32_t length = mrf_read_long(MESSAGE_PAYLOAD_ADDR + 3) | (((uint32_t) mrf_read_long(MESSAGE_PAYLOAD_ADDR + 4)) << 8) | (((uint32_t) mrf_read_long(MESSAGE_PAYLOAD_ADDR + 5)) << 16);
							region_sum = 0;

							flash_assert_cs();
							flash_tx(0x03);
							flash_tx(address >> 16);
							flash_tx(address >> 8);
							flash_tx(address);
							while (length--) {
								region_sum += flash_txrx(0x00);
							}
							flash_deassert_cs();

							region_sum_pending = true;
						}
						break;
				}
			}
		}
	}
	mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00); // RXDECINV = 0; stop inverting receiver and allow further reception
}

static void avr_main(void) __attribute__((noreturn, used));
static void avr_main(void) {
	// Print the device DNA on the debug port
	{
		uint64_t device_dna = device_dna_read();
		sleep_short();
		static const char PREFIX[] = "\n\nDevice DNA: ";
		for (const char *p = PREFIX; *p; ++p) {
			debug_send(*p);
		}
		static const char HEX_DIGITS[] = "0123456789ABCDEF";
		for (uint8_t i = 0; i < 56 / 4; ++i) {
			debug_send(HEX_DIGITS[(device_dna >> 52) & 0xF]);
			device_dna <<= 4;
		}
		debug_send('\n');
	}

	// Initialize the radio
	mrf_init();
	sleep_short();
	mrf_release_reset();
	sleep_short();
	mrf_common_init(CHANNEL, false, PAN, UINT64_C(0xec89d61e8ffd409b));
	while (inb(MRF_CTL) & 0x10);
	mrf_write_short(MRF_REG_SHORT_SADRH, 0);
	mrf_write_short(MRF_REG_SHORT_SADRL, INDEX);
	mrf_analogue_txrx();
	mrf_write_short(MRF_REG_SHORT_INTCON, 0b11110110);

	// Turn on the radio LED
	radio_led_ctl(true);

	// Initialize a tick count
	uint8_t old_ticks = inb(TICKS);
	for(;;) {
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
				if (txstat & 0x01) {
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
#warning implement this
		}

		// Check if an in-progress Flash erase operation has now finished
		if (erasing_flash) {
			flash_assert_cs();
			flash_tx(0x05);
			uint8_t status_register = flash_txrx(0x00);
			flash_deassert_cs();

			if (!(status_register & 0x01)) {
				if (!transmit_busy) {
					// Send notification to host
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 0, 9); // Header length
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 1, 9 + 1); // Frame length
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 2, 0b01100001); // Frame control LSB
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 3, 0b10001000); // Frame control MSB
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 4, tx_seqnum++); // Sequence number
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 5, PAN & 0xFF); // Destination PAN ID LSB
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 6, PAN >> 8); // Destination PAN ID MSB
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 7, 0x00); // Destination address LSB
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 8, 0x01); // Destination address MSB
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 9, INDEX); // Source address LSB
					mrf_write_long(MRF_REG_LONG_TXNFIFO + 10, 0); // Source address MSB

					mrf_write_long(MRF_REG_LONG_TXNFIFO + 11, 0x02); // SPI flash erase finished

					mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);

					transmit_busy = true;
					erasing_flash = false;
				}
			}
		}

		// Check if a flash region sum operation finished and the sum value needs transmitting
		if (region_sum_pending && !transmit_busy) {
			// Send notification to host
#warning once beaconed coordinator mode is working, destination address can be omitted to send to PAN coordinator
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 0, 9); // Header length
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 1, 9 + 5); // Frame length
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 2, 0b01100001); // Frame control LSB
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 3, 0b10001000); // Frame control MSB
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 4, tx_seqnum++); // Sequence number
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 5, PAN & 0xFF); // Destination PAN ID LSB
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 6, PAN >> 8); // Destination PAN ID MSB
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 7, 0x00); // Destination address LSB
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 8, 0x01); // Destination address MSB
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 9, INDEX); // Source address LSB
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 10, 0); // Source address MSB

			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11, 0x03); // SPI region sum
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 12, region_sum);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 13, region_sum >> 8);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 14, region_sum >> 16);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 15, region_sum >> 24);

			mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101);

			transmit_busy = true;
			region_sum_pending = false;
		}
	}
}

