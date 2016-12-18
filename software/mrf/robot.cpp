#include "mrf/robot.h"
#include "mrf/constants.h"
#include "mrf/dongle.h"
#include "util/algorithm.h"
#include "util/codec.h"
#include "util/dprint.h"
#include "util/string.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <utility>
#include <glibmm/main.h>
#include <sigc++/functors/mem_fun.h>

namespace {
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
	
	//this is a hard limit because we are kicking instead of chipping
	//however, 8 meters is a fine chip to so this may stand
#warning hack for kicking when chipping
	const double MAX_KICK_VALUE = 8.0f;
	const double MAX_CHIP_VALUE = 2.0f;
	unsigned int chicker_power_to_pulse_width(double power, bool chip) {
		unsigned int width;
		if (!chip) {
			power = clamp_symmetric(power, MAX_KICK_VALUE);
			width = static_cast<unsigned>(power * 332.7 + 219.8);
		} else {
			power = clamp_symmetric(power, MAX_CHIP_VALUE);
			width = static_cast<unsigned>(835 * power * power + 469.2 * power + 1118.5);
		}
		return clamp(width, 0U, static_cast<unsigned int>(UINT16_MAX));
	}


}

Drive::Dongle &MRFRobot::dongle() {
	return dongle_;
}

const Drive::Dongle &MRFRobot::dongle() const {
	return dongle_;
}

void MRFRobot::set_charger_state(ChargerState state) {
	charger_state = state;
	dirty_drive();
}

void MRFRobot::move_slow(bool slow) {
	assert(!direct_control);
	this->slow = slow;
	dirty_drive();
}

void MRFRobot::move_coast() {
	assert(!direct_control);
	primitive = Drive::Primitive::STOP;
	params[0] = 0.0;
	params[1] = 0.0;
	params[2] = 0.0;
	params[3] = 0.0;
	extra = 0;
	dirty_drive();
}

void MRFRobot::move_brake() {
	assert(!direct_control);
	primitive = Drive::Primitive::STOP;
	params[0] = 0.0;
	params[1] = 0.0;
	params[2] = 0.0;
	params[3] = 0.0;
	extra = 1;
	dirty_drive();
}

void MRFRobot::move_move(Point dest, double time_delta) {
	assert(!direct_control);
	primitive = Drive::Primitive::MOVE;
	params[0] = dest.x * 1000.0;
	params[1] = dest.y * 1000.0;
	params[2] = 0.0;
	params[3] = time_delta * 1000.0;
	extra = 0;
	dirty_drive();
}

void MRFRobot::move_move(Point dest, Angle orientation, double time_delta) {
	assert(!direct_control);
	primitive = Drive::Primitive::MOVE;
	params[0] = dest.x * 1000.0;
	params[1] = dest.y * 1000.0;
	params[2] = orientation.angle_mod().to_radians() * 100.0;
	params[3] = time_delta * 1000.0;
	extra = 1;
	dirty_drive();
}

void MRFRobot::move_dribble(Point dest, Angle orientation, double desired_rpm, bool small_kick_allowed) {
	assert(!direct_control);
	primitive = Drive::Primitive::DRIBBLE;
	params[0] = dest.x * 1000.0;
	params[1] = dest.y * 1000.0;
	params[2] = orientation.angle_mod().to_radians() * 100.0;
	params[3] = desired_rpm;
	extra = small_kick_allowed;
	dirty_drive();
}

void MRFRobot::move_shoot(Point dest, double power, bool chip) {
	assert(!direct_control);
	primitive = Drive::Primitive::SHOOT;
	params[0] = dest.x * 1000.0;
	params[1] = dest.y * 1000.0;
	params[2] = 0.0;
	uint16_t chick_power = static_cast<uint16_t>(chicker_power_to_pulse_width(power,chip));
	params[3] = chick_power;
	extra = chip;
	dirty_drive();
}

void MRFRobot::move_shoot(Point dest, Angle orientation, double power, bool chip) {
	assert(!direct_control);
	primitive = Drive::Primitive::SHOOT;
	params[0] = dest.x * 1000.0;
	params[1] = dest.y * 1000.0;
	params[2] = orientation.angle_mod().to_radians() * 100.0;
	uint16_t chick_power = static_cast<uint16_t>(chicker_power_to_pulse_width(power,chip));
	params[3] = chick_power;
	extra = static_cast<uint8_t>(2 | chip);
	dirty_drive();
}

void MRFRobot::move_catch(Angle angle_diff, double displacement, double speed) {
	assert(!direct_control);
	primitive = Drive::Primitive::CATCH;
	params[0] = angle_diff.angle_mod().to_radians() * 100.0;
	params[1] = displacement * 1000.0;
	params[2] = speed * 1000.0;
	params[3] = 0.0;
	extra = 0;
	dirty_drive();
}

void MRFRobot::move_pivot(Point centre, Angle swing, Angle orientation) {
	assert(!direct_control);
	primitive = Drive::Primitive::PIVOT;
	params[0] = centre.x * 1000.0;
	params[1] = centre.y * 1000.0;
	params[2] = swing.angle_mod().to_radians() * 100.0;
	params[3] = orientation.angle_mod().to_radians() * 100.0;
	extra = 0;
	dirty_drive();
}

void MRFRobot::move_spin(Point dest, Angle speed) {
	assert(!direct_control);
	primitive = Drive::Primitive::SPIN;
	params[0] = dest.x * 1000.0;
	params[1] = dest.y * 1000.0;
	params[2] = speed.to_radians() * 100.0;
	params[3] = 0.0;
	extra = 0;
	dirty_drive();
}

void MRFRobot::direct_wheels(const int (&wheels)[4]) {
	assert(direct_control);
	primitive = Drive::Primitive::DIRECT_WHEELS;
	params[0] = wheels[0];
	params[1] = wheels[1];
	params[2] = wheels[2];
	params[3] = wheels[3];
	dirty_drive();
}

void MRFRobot::direct_velocity(Point vel, Angle avel) {
	assert(direct_control);
	primitive = Drive::Primitive::DIRECT_VELOCITY;
	params[0] = vel.x * 1000.0;
	params[1] = vel.y * 1000.0;
	params[2] = avel.to_radians() * 100.0;
	params[3] = 0.0;
	dirty_drive();
}

void MRFRobot::direct_dribbler(unsigned int rpm) {
	assert(direct_control);
	assert(rpm <= MAX_DRIBBLER_RPM);
	extra = static_cast<uint8_t>(rpm/300);
	dirty_drive();
}

void MRFRobot::direct_chicker(double power, bool chip) {
	uint16_t width = static_cast<uint16_t>(chicker_power_to_pulse_width(power, chip));

	uint8_t buffer[4];
	buffer[0] = 0x00;
	buffer[1] = chip ? 0x01 : 0x00;
	buffer[2] = static_cast<uint8_t>(width);
	buffer[3] = static_cast<uint8_t>(width >> 8);

	dongle_.send_unreliable(index, 20, buffer, sizeof(buffer));
}

void MRFRobot::direct_chicker_auto(double power, bool chip) {
	uint16_t width = static_cast<uint16_t>(chicker_power_to_pulse_width(power, chip));

	if (power > 0.001 && width) {
		uint8_t buffer[4];
		buffer[0] = 0x01;
		buffer[1] = chip ? 0x01 : 0x00;
		buffer[2] = static_cast<uint8_t>(width);
		buffer[3] = static_cast<uint8_t>(width >> 8);

		dongle_.send_unreliable(index, 20, buffer, sizeof(buffer));
	} else {
		uint8_t buffer[1];
		buffer[0] = 0x02;

		dongle_.send_unreliable(index, 20, buffer, sizeof(buffer));
	}
}

constexpr unsigned int MRFRobot::SD_MESSAGE_COUNT;
constexpr unsigned int MRFRobot::LOGGER_MESSAGE_COUNT;

MRFRobot::MRFRobot(MRFDongle &dongle, unsigned int index) :
		Drive::Robot(index, 0.3, 10.0, 2.0, MAX_DRIBBLER_RPM),
		dongle_(dongle),
		low_capacitor_message(Glib::ustring::compose(u8"Bot %1 low caps (fuse blown?)", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::HIGH),
		fw_build_id_mismatch_message(u8"", Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::LOW),
		fpga_build_id_mismatch_message(u8"", Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::LOW),
		build_id_fetch_error_message(Glib::ustring::compose(u8"Bot %1 failed to read build IDs", index), Annunciator::Message::TriggerMode::LEVEL, Annunciator::Message::Severity::LOW),
		charger_state(ChargerState::FLOAT),
		slow(false),
		params{0.0, 0.0, 0.0, 0.0},
		extra(0),
		drive_dirty(false),
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

	direct_control.signal_changed().connect(sigc::mem_fun(this, &MRFRobot::handle_direct_control_changed));
}

MRFRobot::~MRFRobot() {
	feedback_timeout_connection.disconnect();
}

void MRFRobot::encode_drive_packet(void *out) {
	uint16_t words[4];

	// Encode the parameter words.
	for(std::size_t i = 0; i != sizeof(params) / sizeof(*params); ++i) {
		double value = params[i];
		switch (std::fpclassify(value)) {
			case FP_NAN:
				value = 0.0;
				break;
			case FP_INFINITE:
				if (value > 0.0) {
					value = 10000.0;
				} else {
					value = -10000.0;
				}
				break;
		}
		words[i] = 0;
		if (value < 0.0) {
			words[i] |= 1 << 10;
			value = -value;
		}
		if (value > 1000.0) {
			words[i] |= 1 << 11;
			value *= 0.1;
		}
		if (value > 1000.0) {
			value = 1000.0;
		}
		words[i] |= static_cast<uint16_t>(value);
	}

	// Encode the movement primitive number.
	words[0] = static_cast<uint16_t>(words[0] | static_cast<unsigned int>(primitive.get()) << 12);

	// Encode the charger state.
	switch (charger_state) {
		case ChargerState::DISCHARGE:
			words[1] |= 1 << 14;
			break;
		case ChargerState::FLOAT:
			break;
		case ChargerState::CHARGE:
			words[1] |= 2 << 14;
			break;
	}

	// Encode extra data plus the slow flag.
	assert(extra <= 127);
	uint8_t extra_encoded = static_cast<uint8_t>(extra | (slow ? 0x80 : 0x00));

	words[2] = static_cast<uint16_t>(words[2] | static_cast<uint16_t>((extra_encoded & 0xF) << 12));
	words[3] = static_cast<uint16_t>(words[3] | static_cast<uint16_t>((extra_encoded >> 4) << 12));

	// Convert the words to bytes.
	uint8_t *wptr = static_cast<uint8_t *>(out);
	for (std::size_t i = 0; i != 4; ++i) {
		*wptr++ = static_cast<uint8_t>(words[i]);
		*wptr++ = static_cast<uint8_t>(words[i] / 256);
	}
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
									check_build_id_mismatch();
									bptr += 8;
									len -= 8;
								} else {
									LOG_ERROR(Glib::ustring::compose(u8"Received general robot status update with truncated build IDs extension of length %1", len));
								}
								break;

							case 0x02: // LPS data.
								++bptr;
								--len;
								if (len >= 4) {
									for (unsigned int i = 0; i < 4; ++i) {
										lps_values[i] = static_cast<int8_t>(*bptr++) / 10.0;
									}
									len -= 4;

								} else {
									LOG_ERROR(Glib::ustring::compose(u8"Received general robot status update with truncated LPS data extension of length %1", len));
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

				if (!build_ids_valid && request_build_ids_timer.elapsed() > REQUEST_BUILD_IDS_INTERVAL) {
					request_build_ids_timer.stop();
					request_build_ids_timer.reset();
					request_build_ids_timer.start();
					if (request_build_ids_counter) {
						--request_build_ids_counter;
						static const uint8_t REQUEST = 0x0D;
						dongle_.send_unreliable(index, 20, &REQUEST, 1);
					} else {
						build_id_fetch_error_message.active(true);
					}
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
	fw_build_id_mismatch_message.active(false);
	fpga_build_id_mismatch_message.active(false);
	build_id_fetch_error_message.active(false);
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

void MRFRobot::handle_direct_control_changed() {
	if (direct_control) {
		primitive = Drive::Primitive::DIRECT_WHEELS;
	} else {
		primitive = Drive::Primitive::STOP;
	}
	params[0] = 0.0;
	params[1] = 0.0;
	params[2] = 0.0;
	params[3] = 0.0;
	extra = 0;
	dirty_drive();
}

void MRFRobot::dirty_drive() {
	drive_dirty = true;
	//dongle_.dirty_drive();
}

void MRFRobot::check_build_id_mismatch() {
	static const struct {
		const char *name;
		Property<uint32_t> Drive::Robot::*field;
		Annunciator::Message MRFRobot::*message;
	} FIELDS[] = {
		{ u8"FW", &Drive::Robot::fw_build_id, &MRFRobot::fw_build_id_mismatch_message },
		{ u8"FPGA", &Drive::Robot::fpga_build_id, &MRFRobot::fpga_build_id_mismatch_message },
	};

	for (const auto &field : FIELDS) {
		// Build an array of (count of matching robots, build ID) pairs.
		std::array<std::pair<unsigned int, uint32_t>, sizeof(dongle_.robots) / sizeof(*dongle_.robots)> ids;
		decltype(ids)::size_type next = 0;
		for (const std::unique_ptr<MRFRobot> &bot : dongle_.robots) {
			if (bot->build_ids_valid) {
				bool found = false;
				for (decltype(ids)::size_type i = 0; i != next; ++i) {
					if (ids[i].second == bot.get()->*field.field) {
						++ids[i].first;
						found = true;
						break;
					}
				}
				if (!found) {
					ids[next].first = 1;
					ids[next].second = bot.get()->*field.field;
					++next;
				}
			}
		}

		// Sort the array in increasing order of count.
		std::sort(ids.begin(), ids.begin() + next);

		// Activate the warning message on all robots that do not match the majority build ID.
		for (const std::unique_ptr<MRFRobot> &bot : dongle_.robots) {
			if (bot->build_ids_valid && bot.get()->*field.field != ids[next - 1].second) {
				(bot.get()->*field.message).set_text(Glib::ustring::compose(u8"Bot %1 %2 build ID 0x%3 mismatches majority 0x%4", bot->index, field.name, tohex(bot.get()->*field.field, 8), tohex(ids[next - 1].second)));
				(bot.get()->*field.message).active(true);
			} else {
				(bot.get()->*field.message).active(false);
			}
		}
	}
}
