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
#include <functional>
#include <memory>
#include <stdint.h>

class MRFDongle;

/**
 * \brief A single robot addressable through a dongle
 */
class MRFRobot : public Drive::Robot {
	public:
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

		MRFDongle &dongle;
		Annunciator::Message charge_timeout_message;
		std::array<std::unique_ptr<Annunciator::Message>, 10> hall_sensor_stuck_messages;
		std::array<std::unique_ptr<Annunciator::Message>, 4> optical_encoder_not_commutating_messages;

		MRFRobot(MRFDongle &dongle, unsigned int index);
		void handle_message(const void *data, std::size_t len);
};

#endif

