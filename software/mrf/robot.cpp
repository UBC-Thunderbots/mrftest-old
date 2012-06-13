#include "mrf/robot.h"
#include "mrf/dongle.h"
#include "util/dprint.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>

void MRFRobot::drive(const int(&wheels)[4], bool controlled) {
	for (unsigned int i = 0; i < 4; ++i) {
		unsigned int level_u = std::abs(wheels[i]);
		if (level_u > 1023) {
			LOG_ERROR(u8"Wheel setpoint out of range");
			level_u = 1023;
		}
		if (wheels[i] < 0) {
			level_u |= 0x400;
		}
		dongle.drive_packet[index][i] &= static_cast<uint16_t>(~0x7FF);
		dongle.drive_packet[index][i] |= static_cast<uint16_t>(level_u);
	}

	dongle.drive_packet[index][0] |= 1 << 14;
	dongle.drive_packet[index][0] &= static_cast<uint16_t>(~(1 << 13));
	if (controlled) {
		dongle.drive_packet[index][0] |= 1 << 13;
	}

	dongle.dirty_drive();
}

void MRFRobot::drive_coast() {
	dongle.drive_packet[index][0] &= static_cast<uint16_t>(~(3 << 13));
	dongle.dirty_drive();
}

void MRFRobot::drive_brake() {
	dongle.drive_packet[index][0] &= static_cast<uint16_t>(~(1 << 13));
	dongle.drive_packet[index][0] |= 1 << 13;
	dongle.dirty_drive();
}

void MRFRobot::dribble(bool active) {
	dongle.drive_packet[index][0] &= static_cast<uint16_t>(~(1 << 12));
	if (active) {
		dongle.drive_packet[index][0] |= 1 << 12;
	}
	dongle.dirty_drive();
}

void MRFRobot::set_charger_state(ChargerState state) {
	dongle.drive_packet[index][1] &= static_cast<uint16_t>(~(0b11 << 14));
	switch (state) {
		case ChargerState::DISCHARGE:
			break;
		case ChargerState::FLOAT:
			dongle.drive_packet[index][1] |= 0b01 << 14;
			break;
		case ChargerState::CHARGE:
			dongle.drive_packet[index][1] |= 0b10 << 14;
			break;
	}
	dongle.dirty_drive();
}

void MRFRobot::kick(bool chip, unsigned int pulse_width) {
	assert(pulse_width <= 65535);

	uint8_t buffer[4];
	buffer[0] = 0x00;
	buffer[1] = chip ? 0x01 : 0x00;
	buffer[2] = static_cast<uint8_t>(pulse_width);
	buffer[3] = static_cast<uint8_t>(pulse_width >> 8);

	dongle.send_unreliable(index, buffer, sizeof(buffer));
}

void MRFRobot::autokick(bool chip, unsigned int pulse_width) {
	assert(pulse_width <= 65535);

	if (pulse_width) {
		uint8_t buffer[4];
		buffer[0] = 0x01;
		buffer[1] = chip ? 0x01 : 0x00;
		buffer[2] = static_cast<uint8_t>(pulse_width);
		buffer[3] = static_cast<uint8_t>(pulse_width >> 8);

		dongle.send_unreliable(index, buffer, sizeof(buffer));
	} else {
		uint8_t buffer[1];
		buffer[0] = 0x02;

		dongle.send_unreliable(index, buffer, sizeof(buffer));
	}
}

MRFRobot::MRFRobot(MRFDongle &dongle, unsigned int index) : index(index), alive(false), has_feedback(false), ball_in_beam(false), capacitor_charged(false), battery_voltage(0), capacitor_voltage(0), break_beam_reading(0), dribbler_temperature(0), dongle(dongle), hall_stuck_message(Glib::ustring::compose("Bot %1 hall sensor stuck", index), Annunciator::Message::TriggerMode::LEVEL) {
}

void MRFRobot::handle_message(const void *data, std::size_t len) {
	const uint8_t *bptr = static_cast<const uint8_t *>(data);
	if (len) {
		switch (bptr[0]) {
			case 0x00:
				// General robot status update
				++bptr;
				--len;
				if (len == 9) {
					alive = true;
					has_feedback = true;
					battery_voltage = (bptr[0] | (bptr[1] << 8)) / 1024.0 * 3.3 / 3300 * (3300 + 15000);
					capacitor_voltage = (bptr[2] | (bptr[3] << 8)) / 4096.0 * 3.3 / 2200 * (2200 + 220000);
					break_beam_reading = bptr[4] | (bptr[5] << 8);
					dribbler_temperature = (bptr[6] | (bptr[7] << 8)) * 0.5735 - 205.9815;
					ball_in_beam = !!(bptr[8] & 0x01);
					capacitor_charged = !!(bptr[8] & 0x02);
				} else {
					LOG_ERROR(Glib::ustring::compose(u8"Received general robot status update with wrong byte count %1", len));
				}
				break;

			case 0x01:
				// Autokick fired
				signal_autokick_fired.emit();
				break;

			default:
				LOG_ERROR(u8"Received packet from robot with unknown message type");
				break;
		}
	}
}

