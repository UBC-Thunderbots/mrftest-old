#ifndef MRF_ROBOT_H
#define MRF_ROBOT_H

#include "drive/robot.h"
#include "util/annunciator.h"
#include "util/async_operation.h"
#include "util/bit_array.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdint.h>
#include <sigc++/connection.h>

class MRFDongle;

/**
 * \brief A single robot addressable through a dongle
 */
class MRFRobot : public Drive::Robot {
	public:
		Drive::Dongle &dongle();
		const Drive::Dongle &dongle() const;
		void drive(const int(&wheels)[4], bool controlled = true);
		bool can_coast() const;
		void drive_coast_or_manual(const int(&wheels)[4]);
		void drive_brake();
		void dribble(bool active = true);
		void set_charger_state(ChargerState state);
		double kick_pulse_maximum() const;
		double kick_pulse_resolution() const;
		void kick(bool chip, double pulse_width);
		void autokick(bool chip, double pulse_width);

	private:
		friend class MRFDongle;

		MRFDongle &dongle_;
		Annunciator::Message charge_timeout_message, breakout_missing_message, chicker_missing_message, interlocks_overridden_message, low_capacitor_message;
		std::array<std::unique_ptr<Annunciator::Message>, 10> hall_sensor_stuck_messages;
		std::array<std::unique_ptr<Annunciator::Message>, 4> optical_encoder_not_commutating_messages;
		std::array<std::unique_ptr<Annunciator::Message>, 10> sd_messages;
		std::array<std::unique_ptr<Annunciator::Message>, 5> logger_messages;
		std::array<std::unique_ptr<Annunciator::Message>, 5> hot_motor_messages;
		sigc::connection feedback_timeout_connection;

		MRFRobot(MRFDongle &dongle, unsigned int index);
	public:
		~MRFRobot(); // Public only for std::unique_ptr.
	private:
		void handle_message(const void *data, std::size_t len, uint8_t lqi, uint8_t rssi);
		bool handle_feedback_timeout();
};

#endif

