#include "mrf/robot.h"
#include "mrf/dongle.h"
#include "util/algorithm.h"
#include "util/codec.h"
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

	/**
	 * \brief The number of attempts to request the build IDs before giving up.
	 */
	const unsigned int REQUEST_BUILD_IDS_COUNT = 7;

	/**
	 * \brief The number of seconds to wait between consecutive requests for
	 * the build IDs.
	 */
	const double REQUEST_BUILD_IDS_INTERVAL = 0.5;

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

	const char * const SD_MESSAGES[] = {
		nullptr,
		u8"Bot %1 SD card uninitialized",
		nullptr,
		u8"Bot %1 SD card incompatible",
		u8"Bot %1 SD card sent illegal response",
		u8"Bot %1 SD layer logical error",
		u8"Bot %1 SD card CRC error",
		u8"Bot %1 SD card claimed illegal command",
		u8"Bot %1 SD card in unexpected state",
		u8"Bot %1 SD card internal error",
		u8"Bot %1 SD card command response timeout",
		u8"Bot %1 SD card parameter out of range",
		u8"Bot %1 SD card address misaligned",
		u8"Bot %1 SD card block length error",
		u8"Bot %1 SD card erase sequence error",
		u8"Bot %1 SD card erase parameter error",
		u8"Bot %1 SD card write protect violation",
		u8"Bot %1 SD card locked",
		u8"Bot %1 SD card lock or unlock failed",
		u8"Bot %1 SD card command CRC error",
		u8"Bot %1 SD card ECC error",
		u8"Bot %1 SD card CC error",
		u8"Bot %1 SD card generic error",
		u8"Bot %1 SD card CSD write error",
		u8"Bot %1 SD card partial erase due to write protection",
		u8"Bot %1 SD card ECC disabled",
		u8"Bot %1 SD card erase sequence cancelled",
		u8"Bot %1 SD card authentication sequence error",
		u8"Bot %1 SD card initialization timeout",
		u8"Bot %1 SD card data timeout",
		u8"Bot %1 SD card data CRC error",
		u8"Bot %1 SD card missing data start bit",
		u8"Bot %1 SD card FIFO overrun or underrun",
	};

	const char * const LOGGER_MESSAGES[] = {
		nullptr,
		u8"Bot %1 logger uninitialized",
		nullptr,
		u8"Bot %1 SD card full",
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
		pulse_width = static_cast<unsigned>(value * 332.7 + 219.8);
	} else {
		pulse_width =static_cast<unsigned>(835 * value * value + 469.2 * value + 1118.5);
	}
	unsigned int clamped = static_cast<unsigned>(clamp(static_cast<int>(pulse_width + 0.1), 0, static_cast<int>(kick_pulse_maximum())));

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
		pulse_width = static_cast<unsigned>(value * 332.7 + 219.8);
	} else {
		pulse_width = static_cast<unsigned>(835 * value * value + 469.2 * value + 1118.5);
	}
	unsigned int clamped = static_cast<unsigned>(clamp(static_cast<int>(pulse_width + 0.1), 0, static_cast<int>(kick_pulse_maximum())));

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

constexpr unsigned int MRFRobot::SD_MESSAGE_COUNT;
constexpr unsigned int MRFRobot::LOGGER_MESSAGE_COUNT;

MRFRobot::MRFRobot(MRFDongle &dongle, unsigned int index) :
		Drive::Robot(index, DRIBBLE_POWER_MAX),
		dongle_(dongle),
		low_capacitor_message(Glib::ustring::compose(u8"Bot %1 capacitor low (fuse blown?)", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH),
		request_build_ids_counter(REQUEST_BUILD_IDS_COUNT) {
	for (unsigned int i = 0; i < MRF::ERROR_LT_COUNT; ++i) {
		error_lt_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 %2", index, MRF::ERROR_LT_MESSAGES[i]), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH));
	}
	for (unsigned int i = 0; i < MRF::ERROR_ET_COUNT; ++i) {
		error_et_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(u8"Bot %1 %2", index, MRF::ERROR_ET_MESSAGES[i]), Annunciator::Message::TriggerMode::EDGE, Annunciator::Message::Severity::HIGH));
	}

	static_assert(sizeof(SD_MESSAGES) / sizeof(*SD_MESSAGES) == MRFRobot::SD_MESSAGE_COUNT, "Wrong number of SD message initializers");
	for (std::size_t i = 0; i < sizeof(SD_MESSAGES) / sizeof(*SD_MESSAGES); ++i) {
		if (SD_MESSAGES[i]) {
			sd_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(SD_MESSAGES[i], index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH));
		}
	}

	static_assert(sizeof(LOGGER_MESSAGES) / sizeof(*LOGGER_MESSAGES) == MRFRobot::LOGGER_MESSAGE_COUNT, "Wrong number of SD message initializers");
	for (std::size_t i = 0; i < sizeof(LOGGER_MESSAGES) / sizeof(*LOGGER_MESSAGES); ++i) {
		if (LOGGER_MESSAGES[i]) {
			logger_messages[i].reset(new Annunciator::Message(Glib::ustring::compose(LOGGER_MESSAGES[i], index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH));
		}
	}
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
		switch (*bptr) {
			case 0x00:
				// General robot status update
				++bptr;
				--len;
				if (len >= 13) {
					alive = true;

					battery_voltage = (bptr[0] | static_cast<unsigned int>(bptr[1] << 8)) / 1000.0;
					bptr += 2;
					len -= 2;

					capacitor_voltage = (bptr[0] | static_cast<unsigned int>(bptr[1] << 8)) / 100.0;
					low_capacitor_message.active(capacitor_voltage < 5.0);
					bptr += 2;
					len -= 2;

					break_beam_reading = (bptr[0] | static_cast<unsigned int>(bptr[1] << 8)) / 1000.0;
					break_beam_scale = 0.3;
					bptr += 2;
					len -= 2;

					board_temperature = (bptr[0] | static_cast<unsigned int>(bptr[1] << 8)) / 100.0;
					bptr += 2;
					len -= 2;

					ball_in_beam = !!(*bptr & 0x80);
					capacitor_charged = !!(*bptr & 0x40);
					unsigned int logger_status = *bptr & 0x3F;
					for (std::size_t i = 0; i < logger_messages.size(); ++i) {
						if (logger_messages[i]) {
							logger_messages[i]->active(logger_status == i);
						}
					}
					++bptr;
					--len;

					for (std::size_t i = 0; i < sd_messages.size(); ++i) {
						if (sd_messages[i]) {
							sd_messages[i]->active(*bptr == i);
						}
					}
					++bptr;
					--len;

					dribbler_speed = static_cast<int16_t>(static_cast<uint16_t>(bptr[0] | (bptr[1] << 8))) * 25 * 60 / 6;
					bptr += 2;
					len -= 2;

					dribbler_temperature = *bptr++;
					--len;

					bool has_error_extension = false;
					while (len) {
						// Decode extensions.
						switch (*bptr) {
							case 0x00: // Error bits.
								++bptr;
								--len;
								if (len >= MRF::ERROR_BYTES) {
									has_error_extension = true;
									for (unsigned int i = 0; i != MRF::ERROR_LT_COUNT; ++i) {
										unsigned int byte = i / CHAR_BIT;
										unsigned int bit = i % CHAR_BIT;
										error_lt_messages[i]->active(bptr[byte] & (1 << bit));
									}
									for (unsigned int i = 0; i != MRF::ERROR_ET_COUNT; ++i) {
										unsigned int byte = (i + MRF::ERROR_LT_COUNT) / CHAR_BIT;
										unsigned int bit = (i + MRF::ERROR_LT_COUNT) % CHAR_BIT;
										if (bptr[byte] & (1 << bit)) {
											error_et_messages[i]->fire();
										}
									}
									bptr += MRF::ERROR_BYTES;
									len -= MRF::ERROR_BYTES;
								} else {
									LOG_ERROR(Glib::ustring::compose(u8"Received general robot status update with truncated error bits extension of length %1", len));
								}
								break;

							case 0x01: // Build IDs.
								++bptr;
								--len;
								if (len >= 8) {
									build_ids_valid = true;
									fw_build_id = decode_u32_le(bptr);
									fpga_build_id = decode_u32_le(bptr + 4);
									bptr += 8;
									len -= 8;
								} else {
									LOG_ERROR(Glib::ustring::compose(u8"Received general robot status update with truncated build IDs extension of length %1", len));
								}
								break;

							default:
								LOG_ERROR(Glib::ustring::compose(u8"Received general status packet from robot with unknown extension code %1", static_cast<unsigned int>(*bptr)));
								len = 0;
								break;
						}
					}

					if (!has_error_extension) {
						// Error reporting extension is absent → no errors are
						// asserted.
						for (auto &i : error_lt_messages) {
							i->active(false);
						}
					}

					feedback_timeout_connection.disconnect();
					feedback_timeout_connection = Glib::signal_timeout().connect_seconds(sigc::mem_fun(this, &MRFRobot::handle_feedback_timeout), 3);
				} else {
					LOG_ERROR(Glib::ustring::compose(u8"Received general robot status update with wrong byte count %1", len));
				}

				if (!build_ids_valid && request_build_ids_counter && request_build_ids_timer.elapsed() > REQUEST_BUILD_IDS_INTERVAL) {
					--request_build_ids_counter;
					request_build_ids_timer.stop();
					request_build_ids_timer.reset();
					request_build_ids_timer.start();
					static const uint8_t REQUEST = 0x0D;
					dongle_.send_unreliable(index, &REQUEST, 1);
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
				LOG_ERROR(Glib::ustring::compose(u8"Received packet from robot with unknown message type %1", static_cast<unsigned int>(*bptr)));
				break;
		}
	}
}

bool MRFRobot::handle_feedback_timeout() {
	alive = false;
	build_ids_valid = false;
	request_build_ids_counter = REQUEST_BUILD_IDS_COUNT;
	low_capacitor_message.active(false);
	for (auto &i : error_lt_messages) {
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
	return false;
}
