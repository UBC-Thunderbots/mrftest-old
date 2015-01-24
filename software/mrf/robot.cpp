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
	const unsigned int DRIBBLE_POWER_BITS = 5U;
	const unsigned int DRIBBLE_POWER_MAX = (1U << DRIBBLE_POWER_BITS) - 1U;

	struct RSSITableEntry final {
		int rssi;
		int db;
	};
	const struct RSSITableEntry RSSI_TABLE[] = {
		{ 255, -35 },
		{ 254, -36 },
		{ 253, -37 },
		{ 250, -38 },
		{ 245, -39 },
		{ 239, -40 },
		{ 233, -41 },
		{ 228, -42 },
		{ 225, -43 },
		{ 221, -44 },
		{ 216, -45 },
		{ 212, -46 },
		{ 207, -47 },
		{ 203, -48 },
		{ 198, -49 },
		{ 193, -50 },
		{ 188, -51 },
		{ 183, -52 },
		{ 176, -53 },
		{ 170, -54 },
		{ 165, -55 },
		{ 159, -56 },
		{ 153, -57 },
		{ 148, -58 },
		{ 143, -59 },
		{ 138, -60 },
		{ 133, -61 },
		{ 129, -62 },
		{ 125, -63 },
		{ 121, -64 },
		{ 117, -65 },
		{ 111, -66 },
		{ 107, -67 },
		{ 100, -68 },
		{ 95, -69 },
		{ 89, -70 },
		{ 83, -71 },
		{ 78, -72 },
		{ 73, -73 },
		{ 68, -74 },
		{ 63, -75 },
		{ 58, -76 },
		{ 53, -77 },
		{ 48, -78 },
		{ 43, -79 },
		{ 37, -80 },
		{ 32, -81 },
		{ 27, -82 },
		{ 23, -83 },
		{ 18, -84 },
		{ 13, -85 },
		{ 9, -86 },
		{ 5, -87 },
		{ 2, -88 },
		{ 1, -89 },
		{ 0, -90 },
	};

	struct MessageTemplate final {
		const char *pattern;
		Annunciator::Message::Severity severity;
	};

	const MessageTemplate SD_MESSAGES[] = {
		{ nullptr, Annunciator::Message::Severity::LOW },
		{ u8"Bot %1 SD card uninitialized", Annunciator::Message::Severity::HIGH },
		{ nullptr, Annunciator::Message::Severity::LOW },
		{ u8"Bot %1 SD card incompatible", Annunciator::Message::Severity::HIGH },
		{ u8"Bot %1 SD card sent illegal response", Annunciator::Message::Severity::HIGH },
		{ u8"Bot %1 SD layer logical error", Annunciator::Message::Severity::HIGH },
		{ u8"Bot %1 SD card CRC error", Annunciator::Message::Severity::HIGH },
		{ u8"Bot %1 SD card claimed illegal command", Annunciator::Message::Severity::HIGH },
		{ u8"Bot %1 SD card illegally idle", Annunciator::Message::Severity::HIGH },
		{ u8"Bot %1 SD card internal error", Annunciator::Message::Severity::HIGH },
	};

	const MessageTemplate LOGGER_MESSAGES[] = {
		{ nullptr, Annunciator::Message::Severity::LOW },
		{ u8"Bot %1 logger uninitialized", Annunciator::Message::Severity::HIGH },
		{ nullptr, Annunciator::Message::Severity::LOW },
		{ u8"Bot %1 SD card full", Annunciator::Message::Severity::HIGH },
	};
}

Drive::Dongle &MRFRobot::dongle() {
	return dongle_;
}

const Drive::Dongle &MRFRobot::dongle() const {
	return dongle_;
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
		dongle_.drive_packet[index][i] &= static_cast<uint16_t>(~0x7FF);
		dongle_.drive_packet[index][i] |= static_cast<uint16_t>(level_u);
	}

	dongle_.drive_packet[index][0] |= 1 << 14;
	dongle_.drive_packet[index][0] &= static_cast<uint16_t>(~(1 << 13));
	if (controlled) {
		dongle_.drive_packet[index][0] |= 1 << 13;
	}

	dongle_.dirty_drive();
}

bool MRFRobot::can_coast() const {
	return true;
}

void MRFRobot::drive_coast() {
	dongle_.drive_packet[index][0U] &= static_cast<uint16_t>(~(3U << 13U));
	dongle_.dirty_drive();
}

void MRFRobot::drive_brake() {
	dongle_.drive_packet[index][0] &= static_cast<uint16_t>(~(3 << 13));
	dongle_.drive_packet[index][0] |= 1 << 13;
	dongle_.dirty_drive();
}

void MRFRobot::dribble(unsigned int power) {
	power = clamp(power, 0U, DRIBBLE_POWER_MAX);
	dongle_.drive_packet[index][2U] &= static_cast<uint16_t>(~(DRIBBLE_POWER_MAX << 11U));
	dongle_.drive_packet[index][2U] |= static_cast<uint16_t>(power << 11U);
	dongle_.dirty_drive();
}

void MRFRobot::set_charger_state(ChargerState state) {
	dongle_.drive_packet[index][1] &= static_cast<uint16_t>(~(0b11 << 14));
	switch (state) {
		case ChargerState::FLOAT:
			break;
		case ChargerState::DISCHARGE:
			dongle_.drive_packet[index][1] |= 0b01 << 14;
			break;
		case ChargerState::CHARGE:
			dongle_.drive_packet[index][1] |= 0b10 << 14;
			break;
	}
	dongle_.dirty_drive();
}

double MRFRobot::kick_pulse_maximum() const {
	return 5000.0;
}

// To be used for the slide bar
double MRFRobot::kick_speed_maximum() const {
	return 10.0;
}

double MRFRobot::chip_distance_maximum() const {
	return 2.0;
}

double MRFRobot::chip_distance_resolution() const {
	return 1.0;
}

double MRFRobot::kick_pulse_resolution() const {
	return 1.0;
}

// actually currently taking in a velocity
void MRFRobot::kick(bool chip, double value) {
	unsigned int pulse_width;
	if (!chip) {
		pulse_width = value * 332.7 + 219.8;
	} else {
		//ballpark, not real
		pulse_width = 835 * value * value + 469.2 * value + 1118.5;
	}
	unsigned int clamped = static_cast<unsigned>(clamp(static_cast<int>(pulse_width + 0.1), 0, (int) kick_pulse_maximum()));

	uint8_t buffer[4];
	buffer[0] = 0x00;
	buffer[1] = chip ? 0x01 : 0x00;
	buffer[2] = static_cast<uint8_t>(clamped);
	buffer[3] = static_cast<uint8_t>(clamped >> 8);

	dongle_.send_unreliable(index, buffer, sizeof(buffer));
}

void MRFRobot::autokick(bool chip, double value) {
	unsigned int pulse_width; 
	if (!chip) {
		pulse_width = value * 332.7 + 219.8;
	} else {
		pulse_width = value * 1000;
	}
	unsigned int clamped = static_cast<unsigned>(clamp(static_cast<int>(pulse_width + 0.1), 0, (int) kick_pulse_maximum()));

	if (clamped) {
		uint8_t buffer[4];
		buffer[0] = 0x01;
		buffer[1] = chip ? 0x01 : 0x00;
		buffer[2] = static_cast<uint8_t>(clamped);
		buffer[3] = static_cast<uint8_t>(clamped >> 8);

		dongle_.send_unreliable(index, buffer, sizeof(buffer));
	} else {
		uint8_t buffer[1];
		buffer[0] = 0x02;

		dongle_.send_unreliable(index, buffer, sizeof(buffer));
	}
}

MRFRobot::MRFRobot(MRFDongle &dongle, unsigned int index) :
		Drive::Robot(index, DRIBBLE_POWER_MAX),
		dongle_(dongle),
		charge_timeout_message(Glib::ustring::compose(u8"Bot %1 charge timeout", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH),
		breakout_missing_message(Glib::ustring::compose(u8"Bot %1 breakout missing", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::LOW),
		chicker_missing_message(Glib::ustring::compose(u8"Bot %1 chicker missing", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::LOW),
		crc_error_message(Glib::ustring::compose(u8"Bot %1 ICB CRC error", index), Annunciator::Message::TriggerMode::EDGE, Annunciator::Message::Severity::HIGH),
		interlocks_overridden_message(Glib::ustring::compose(u8"Bot %1 interlocks overridden", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH),
		low_capacitor_message(Glib::ustring::compose(u8"Bot %1 low caps (fuse blown?)", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH),
		receive_fcs_fail_message(Glib::ustring::compose(u8"Bot %1 receive FCS fail", index), Annunciator::Message::TriggerMode::EDGE, Annunciator::Message::Severity::HIGH) {
	for (unsigned int i = 0; i < 8; ++i) {
		hall_sensor_stuck_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 wheel %2 Hall sensor stuck %3", index, i / 2, (i % 2) == 0 ? u8"low" : u8"high"), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH));
	}
	hall_sensor_stuck_messages[8].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 dribbler Hall sensor stuck low", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH));
	hall_sensor_stuck_messages[9].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 dribbler Hall sensor stuck high", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH));
	for (unsigned int i = 0; i < 4; ++i) {
		optical_encoder_not_commutating_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 wheel %2 optical encoder not commutating", index, i), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH));
	}
	for (std::size_t i = 0; i < sizeof(SD_MESSAGES) / sizeof(*SD_MESSAGES); ++i) {
		if (SD_MESSAGES[i].pattern) {
			sd_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(SD_MESSAGES[i].pattern, index), Annunciator::Message::TriggerMode::LEVEL, SD_MESSAGES[i].severity));
		}
	}
	for (std::size_t i = 0; i < sizeof(LOGGER_MESSAGES) / sizeof(*LOGGER_MESSAGES); ++i) {
		if (LOGGER_MESSAGES[i].pattern) {
			logger_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(LOGGER_MESSAGES[i].pattern, index), Annunciator::Message::TriggerMode::LEVEL, LOGGER_MESSAGES[i].severity));
		}
	}
	for (std::size_t i = 0; i < 4; ++i) {
		hot_motor_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 wheel %2 motor hot", index, i), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH));
	}
	hot_motor_messages[4].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 dribbler motor hot", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH));
}

MRFRobot::~MRFRobot() {
	feedback_timeout_connection.disconnect();
}

void MRFRobot::handle_message(const void *data, std::size_t len, uint8_t lqi, uint8_t rssi) {
	link_quality = lqi / 255.0;
	{
		bool found = false;
		for (std::size_t i = 0; !found && i < sizeof(RSSI_TABLE) / sizeof(*RSSI_TABLE); ++i) {
			if (RSSI_TABLE[i].rssi < rssi) {
				received_signal_strength = RSSI_TABLE[i].db;
				found = true;
			}
		}
		if (!found) {
			received_signal_strength = -90;
		}
	}

	const uint8_t *bptr = static_cast<const uint8_t *>(data);
	if (len) {
		switch (bptr[0]) {
			case 0x00:
				// General robot status update
				++bptr;
				--len;
				if (len >= 15) {
					alive = true;
					battery_voltage = (bptr[0] | static_cast<unsigned int>(bptr[1] << 8)) / 1000.0;
					capacitor_voltage = (bptr[2] | static_cast<unsigned int>(bptr[3] << 8)) / 100.0;
					break_beam_reading = (bptr[4] | static_cast<unsigned int>(bptr[5] << 8)) / 1000.0;
					break_beam_scale = 0.3;
					board_temperature = (bptr[6] | static_cast<unsigned int>(bptr[7] << 8)) / 100.0;
					ball_in_beam = !!(bptr[8] & 0x01);
					capacitor_charged = !!(bptr[8] & 0x02);
					charge_timeout_message.active(!!(bptr[8] & 0x04));
					bool breakout_present = !!(bptr[8] & 0x08);
					breakout_missing_message.active(!breakout_present);
					bool chicker_present = !!(bptr[8] & 0x10);
					chicker_missing_message.active(!chicker_present);
					if (bptr[8] & 0x20) {
						crc_error_message.fire();
					}
					interlocks_overridden_message.active(!!(bptr[8] & 0x40));
					low_capacitor_message.active(chicker_present && capacitor_voltage < 5);
					for (unsigned int bit = 0; bit < 8; ++bit) {
						hall_sensor_stuck_messages[bit]->active(!!(bptr[9] & (1 << bit)) && breakout_present);
					}
					for (unsigned int bit = 0; bit < 2; ++bit) {
						hall_sensor_stuck_messages[bit + 8]->active(!!(bptr[10] & (1 << bit)));
					}
					for (unsigned int wheel = 0; wheel < 4; ++wheel) {
						optical_encoder_not_commutating_messages[wheel]->active(!!(bptr[10] & (1 << (wheel + 2))) && breakout_present);
					}
					if (bptr[10] & (1 << 6)) {
						receive_fcs_fail_message.fire();
					}
					unsigned int sd_status = bptr[11] & 0x0F;
					unsigned int logger_status = bptr[11] >> 4;
					for (std::size_t i = 0; i < sd_messages.size(); ++i) {
						if (sd_messages[i]) {
							sd_messages[i]->active(sd_status == i);
						}
					}
					for (std::size_t i = 0; i < logger_messages.size(); ++i) {
						if (logger_messages[i]) {
							logger_messages[i]->active(sd_status == 0 && logger_status == i);
						}
					}
					dribbler_speed = static_cast<int16_t>(static_cast<uint16_t>(bptr[12] | (bptr[13] << 8))) * 25 * 60 / 6;
					for (std::size_t i = 0; i < hot_motor_messages.size(); ++i) {
						hot_motor_messages[i]->active(!!(bptr[14] & (1 << i)));
					}
					if (len >= 16) {
						dribbler_temperature = bptr[15];
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

			case 0x04:
				// Robot has ball
				ball_in_beam = true;
				break;

			case 0x05:
				// Robot does not have ball
				ball_in_beam = false;
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
	interlocks_overridden_message.active(false);
	low_capacitor_message.active(false);
	for (auto &i : hall_sensor_stuck_messages) {
		i->active(false);
	}
	for (auto &i : optical_encoder_not_commutating_messages) {
		i->active(false);
	}
	for (auto &i : sd_messages) {
		if (i) {
			i->active(false);
		}
	}
	for (auto &i : logger_messages) {
		if (i) {
			i->active(false);
		}
	}
	for (auto &i : hot_motor_messages) {
		i->active(false);
	}
	return false;
}

