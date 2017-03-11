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
		void set_charger_state(ChargerState state) override;

		void move_slow(bool slow) override;
		void move_coast() override;
		void move_brake() override;
		void move_move(Point dest) override;
		void move_move(Point dest, Angle orientation) override;
		void move_move(Point dest, double time_delta) override;
		void move_move(Point dest, Angle orientation, double time_delta) override;
		void move_dribble(Point dest, Angle orientation, double desired_rpm, bool small_kick_allowed) override;
		void move_shoot(Point dest, double power, bool chip) override;
		void move_shoot(Point dest, Angle orientation, double power, bool chip) override;
		void move_catch(Angle angle_diff, double displacement, double speed) override;
		void move_pivot(Point centre, Angle swing, Angle orientation) override;
		void move_spin(Point dest, Angle speed) override;

		void direct_wheels(const int (&wheels)[4]) override;
		void direct_velocity(Point vel, Angle avel) override;
		void direct_dribbler(unsigned int rpm) override;
		void direct_chicker(double power, bool chip) override;
		void direct_chicker_auto(double power, bool chip) override;

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
		ChargerState charger_state;
		bool slow;
		double params[4];
		uint8_t extra;
		bool drive_dirty;
		Glib::Timer request_build_ids_timer;
		unsigned int request_build_ids_counter;

		explicit MRFRobot(MRFDongle &dongle, unsigned int index);
	public:
		~MRFRobot(); // Public only for std::unique_ptr.
	private:
		void encode_drive_packet(void *out);
		void send_primitive(uint16_t primitive);
		void handle_message(const void *data, std::size_t len, uint8_t lqi, uint8_t rssi);
		bool handle_feedback_timeout();
		void handle_direct_control_changed();
		void dirty_drive();
		void check_build_id_mismatch();
};



inline void MRFRobot::move_move(Point dest) {
	move_move(dest, 0.0);
}

inline void MRFRobot::move_move(Point dest, Angle orientation) {
	move_move(dest, orientation, 0.0);
}

#endif
