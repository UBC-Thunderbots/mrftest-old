#ifndef MRF_ROBOT_H
#define MRF_ROBOT_H

#include "drive/robot.h"
#include "util/annunciator.h"
#include "util/async_operation.h"
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
class MRFRobot final : public Drive::Robot {
	public:
		Drive::Dongle &dongle() override;
		const Drive::Dongle &dongle() const override;
		void drive(const int(&wheels)[4], bool controlled = true) override;
		bool can_coast() const override;
		void drive_coast() override;
		void drive_brake() override;
		void dribble(unsigned int power) override;
		void set_charger_state(ChargerState state) override;
		double kick_pulse_maximum() const override;
		double kick_speed_maximum() const override;
		double kick_pulse_resolution() const override;
		double chip_distance_maximum() const override;
		double chip_distance_resolution() const override;
		void kick(bool chip, double pulse_width) override;
		void autokick(bool chip, double pulse_width) override;

	private:
		friend class MRFDongle;

		MRFDongle &dongle_;
		Annunciator::Message charge_timeout_message, breakout_missing_message, chicker_missing_message, crc_error_message, interlocks_overridden_message, low_capacitor_message, receive_fcs_fail_message;
		std::array<std::unique_ptr<Annunciator::Message>, 10> hall_sensor_stuck_messages;
		std::array<std::unique_ptr<Annunciator::Message>, 4> optical_encoder_not_commutating_messages;
		std::array<std::unique_ptr<Annunciator::Message>, 10> sd_messages;
		std::array<std::unique_ptr<Annunciator::Message>, 4> logger_messages;
		std::array<std::unique_ptr<Annunciator::Message>, 5> hot_motor_messages;
		sigc::connection feedback_timeout_connection;

		explicit MRFRobot(MRFDongle &dongle, unsigned int index);
	public:
		~MRFRobot(); // Public only for std::unique_ptr.
	private:
		void handle_message(const void *data, std::size_t len, uint8_t lqi, uint8_t rssi);
		bool handle_feedback_timeout();
};

#endif

