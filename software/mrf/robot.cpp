#include "mrf/robot.h"
#include "mrf/dongle.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <glibmm/main.h>
#include <sigc++/functors/mem_fun.h>

namespace {
	double adc_voltage_to_board_temp(double voltage) {
		// For V being ADC voltage and R being thermistor voltage:
		// V = 3.3 / (10,000 + R) * R
		// 10,000 V + VR = 3.3 R
		// (3.3 - V) R = 10,000 V
		// R = 10,000 V / (3.3 - V)
		double thermistor_resistance = 10000 * voltage / (3.3 - voltage);

		// Magic math from binaryblade
		double ltemp = std::log(thermistor_resistance);
		double temperature = 1.6648 * ltemp * ltemp - 61.3664 * ltemp + 510.18;

		return temperature;
	}
}

void MRFRobot::drive(const int(&wheels)[4], bool controlled) {
	for (unsigned int i = 0; i < 4; ++i) {
		unsigned int level_u = static_cast<unsigned int>(std::abs(wheels[i]));
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

bool MRFRobot::can_coast() const {
	return true;
}

void MRFRobot::drive_coast_or_manual(const int(&wheels)[4]) {
	for (unsigned int i = 0; i < 4; ++i) {
		unsigned int level_u = static_cast<unsigned int>(std::abs(wheels[i]));
		if (level_u > 255) {
			LOG_ERROR(u8"Wheel PWM duty cycle out of range");
			level_u = 255;
		}
		dongle.drive_packet[index][i] &= static_cast<uint16_t>(~0x7FF);
		dongle.drive_packet[index][i] |= static_cast<uint16_t>(level_u);
	}

	dongle.drive_packet[index][0] &= static_cast<uint16_t>(~(3 << 13));
	dongle.dirty_drive();
}

void MRFRobot::drive_brake() {
	dongle.drive_packet[index][0] &= static_cast<uint16_t>(~(3 << 13));
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
		case ChargerState::FLOAT:
			break;
		case ChargerState::DISCHARGE:
			dongle.drive_packet[index][1] |= 0b01 << 14;
			break;
		case ChargerState::CHARGE:
			dongle.drive_packet[index][1] |= 0b10 << 14;
			break;
	}
	dongle.dirty_drive();
}

double MRFRobot::kick_pulse_maximum() const {
	return 16383.0;
}

double MRFRobot::kick_pulse_resolution() const {
	return 0.25;
}

void MRFRobot::kick(bool chip, double pulse_width) {
	unsigned int clamped = clamp(static_cast<int>(pulse_width * 4.0 + 0.1), 0, 65535);

	uint8_t buffer[4];
	buffer[0] = 0x00;
	buffer[1] = chip ? 0x01 : 0x00;
	buffer[2] = static_cast<uint8_t>(clamped);
	buffer[3] = static_cast<uint8_t>(clamped >> 8);

	dongle.send_unreliable(index, buffer, sizeof(buffer));
}

void MRFRobot::autokick(bool chip, double pulse_width) {
	unsigned int clamped = clamp(static_cast<int>(pulse_width * 4.0 + 0.1), 0, 65535);

	if (clamped) {
		uint8_t buffer[4];
		buffer[0] = 0x01;
		buffer[1] = chip ? 0x01 : 0x00;
		buffer[2] = static_cast<uint8_t>(clamped);
		buffer[3] = static_cast<uint8_t>(clamped >> 8);

		dongle.send_unreliable(index, buffer, sizeof(buffer));
	} else {
		uint8_t buffer[1];
		buffer[0] = 0x02;

		dongle.send_unreliable(index, buffer, sizeof(buffer));
	}
}

MRFRobot::MRFRobot(MRFDongle &dongle, unsigned int index) : Drive::Robot(index), dongle(dongle), charge_timeout_message(Glib::ustring::compose(u8"Bot %1 charge timeout", index), Annunciator::Message::TriggerMode::LEVEL), breakout_missing_message(Glib::ustring::compose(u8"Bot %1 breakout missing", index), Annunciator::Message::TriggerMode::LEVEL), chicker_missing_message(Glib::ustring::compose(u8"Bot %1 chicker missing", index), Annunciator::Message::TriggerMode::LEVEL), sd_missing_message(Glib::ustring::compose(u8"Bot %1 SD card missing", index), Annunciator::Message::TriggerMode::LEVEL), interlocks_overridden_message(Glib::ustring::compose(u8"Bot %1 interlocks overridden", index), Annunciator::Message::TriggerMode::LEVEL) {
	for (unsigned int i = 0; i < 8; ++i) {
		hall_sensor_stuck_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 wheel %2 Hall sensor stuck %3", index, i / 2, (i % 2) == 0 ? u8"low" : u8"high"), Annunciator::Message::TriggerMode::LEVEL));
	}
	hall_sensor_stuck_messages[8].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 dribbler Hall sensor stuck low", index), Annunciator::Message::TriggerMode::LEVEL));
	hall_sensor_stuck_messages[9].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 dribbler Hall sensor stuck high", index), Annunciator::Message::TriggerMode::LEVEL));
	for (unsigned int i = 0; i < 4; ++i) {
		optical_encoder_not_commutating_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 wheel %2 optical encoder not commutating", index, i), Annunciator::Message::TriggerMode::LEVEL));
	}
}

MRFRobot::~MRFRobot() {
	feedback_timeout_connection.disconnect();
}

void MRFRobot::handle_message(const void *data, std::size_t len) {
	const uint8_t *bptr = static_cast<const uint8_t *>(data);
	if (len) {
		switch (bptr[0]) {
			case 0x00:
				// General robot status update
				++bptr;
				--len;
				if (len == 11) {
					alive = true;
					battery_voltage = (bptr[0] | static_cast<unsigned int>(bptr[1] << 8)) / 1024.0 * 3.3 / 2200 * (2200 + 20000);
					capacitor_voltage = (bptr[2] | static_cast<unsigned int>(bptr[3] << 8)) / 1024.0 * 3.3 / 2200 * (2200 + 200000);
					break_beam_reading = static_cast<int16_t>(static_cast<uint16_t>(bptr[4] | static_cast<unsigned int>(bptr[5] << 8)));
					board_temperature = adc_voltage_to_board_temp((bptr[6] | static_cast<unsigned int>(bptr[7] << 8)) / 1024.0 * 3.3);
					ball_in_beam = !!(bptr[8] & 0x01);
					capacitor_charged = !!(bptr[8] & 0x02);
					charge_timeout_message.active(!!(bptr[8] & 0x04));
					bool breakout_present = !!(bptr[8] & 0x08);
					breakout_missing_message.active(!breakout_present);
					chicker_missing_message.active(!(bptr[8] & 0x10));
					sd_missing_message.active(!(bptr[8] & 0x20));
					interlocks_overridden_message.active(!!(bptr[8] & 0x40));
					for (unsigned int bit = 0; bit < 8; ++bit) {
						hall_sensor_stuck_messages[bit]->active(!!(bptr[9] & (1 << bit)) && breakout_present);
					}
					for (unsigned int bit = 0; bit < 2; ++bit) {
						hall_sensor_stuck_messages[bit + 8]->active(!!(bptr[10] & (1 << bit)));
					}
					for (unsigned int wheel = 0; wheel < 4; ++wheel) {
						optical_encoder_not_commutating_messages[wheel]->active(!!(bptr[10] & (1 << (wheel + 2))) && breakout_present);
					}
					feedback_timeout_connection.disconnect();
					feedback_timeout_connection = Glib::signal_timeout().connect_seconds(sigc::mem_fun(this, &MRFRobot::handle_feedback_timeout), 3);
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

bool MRFRobot::handle_feedback_timeout() {
	alive = false;
	charge_timeout_message.active(false);
	breakout_missing_message.active(false);
	chicker_missing_message.active(false);
	sd_missing_message.active(false);
	interlocks_overridden_message.active(false);
	for (auto &i : hall_sensor_stuck_messages) {
		i->active(false);
	}
	for (auto &i : optical_encoder_not_commutating_messages) {
		i->active(false);
	}
	return false;
}

