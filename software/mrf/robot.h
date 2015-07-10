#ifndef MRF_ROBOT_H
#define MRF_ROBOT_H

#include "drive/robot.h"
#include "mrf/constants.h"
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
#include <glibmm/timer.h>
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

		static constexpr unsigned int SD_MESSAGE_COUNT = 33;
		static constexpr unsigned int LOGGER_MESSAGE_COUNT = 4;

		MRFDongle &dongle_;
		Annunciator::Message low_capacitor_message;
		Annunciator::Message fw_build_id_mismatch_message, fpga_build_id_mismatch_message, build_id_fetch_error_message;
		std::array<std::unique_ptr<Annunciator::Message>, MRF::ERROR_LT_COUNT> error_lt_messages;
		std::array<std::unique_ptr<Annunciator::Message>, MRF::ERROR_ET_COUNT> error_et_messages;
		std::array<std::unique_ptr<Annunciator::Message>, SD_MESSAGE_COUNT> sd_messages;
		std::array<std::unique_ptr<Annunciator::Message>, LOGGER_MESSAGE_COUNT> logger_messages;
		sigc::connection feedback_timeout_connection;
		Glib::Timer request_build_ids_timer;
		unsigned int request_build_ids_counter;

		explicit MRFRobot(MRFDongle &dongle, unsigned int index);
	public:
		~MRFRobot(); // Public only for std::unique_ptr.
	private:
		void handle_message(const void *data, std::size_t len, uint8_t lqi, uint8_t rssi);
		bool handle_feedback_timeout();
		void check_build_id_mismatch();
};

#endif
