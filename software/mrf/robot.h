#ifndef MRF_ROBOT_H
#define MRF_ROBOT_H

#include "util/annunciator.h"
#include "util/async_operation.h"
#include "util/bit_array.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include "mrf/dongle.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <stdint.h>

/**
 * \brief A single robot addressable through a dongle
 */
class MRFRobot : public NonCopyable {
	public:
		/**
		 * \brief The possible states of the charger
		 */
		enum class ChargerState {
			/**
			 * \brief Safely discharges the capacitor down to battery level
			 */
			DISCHARGE,

			/**
			 * \brief Neither charges nor discharges the capacitors
			 */
			FLOAT,

			/**
			 * \brief Charges the capacitor to full voltage
			 */
			CHARGE,
		};

		/**
		 * \brief The index of the robot, from 0 to 7
		 */
		const unsigned int index;

		/**
		 * \brief Whether or not the robot is currently responding to radio communication
		 */
		Property<bool> alive;

		/**
		 * \brief Whether or not up-to-date feedback is available for the robot
		 */
		Property<bool> has_feedback;

		/**
		 * \brief Whether or not the ball is interrupting the robot’s laser beam
		 */
		Property<bool> ball_in_beam;

		/**
		 * \brief Whether or not the robot’s capacitor is charged enough to kick the ball
		 */
		Property<bool> capacitor_charged;

		/**
		 * \brief The voltage on the robot’s battery, in volts
		 */
		Property<double> battery_voltage;

		/**
		 * \brief The voltage on the robot’s kicking capacitor, in volts
		 */
		Property<double> capacitor_voltage;

		/**
		 * \brief The raw analogue-to-digital converter reading of the robot’s laser sensor
		 */
		Property<unsigned int> break_beam_reading;

		/**
		 * \brief The temperature of the robot’s dribbler motor, in degrees Celsius
		 */
		Property<double> dribbler_temperature;

		/**
		 * \brief Emitted when the autokick mechanism causes the robot to kick
		 */
		sigc::signal<void> signal_autokick_fired;

		/**
		 * \brief Sets the speeds of the robot’s wheels
		 *
		 * \param[in] wheels the speeds of the wheels, in quarters of a degree per five milliseconds, in the range ±1023
		 *
		 * \param[in] controlled \c true to run the provided setpoints through the wheel controllers, or \c false to run open-loop
		 */
		void drive(const int(&wheels)[4], bool controlled = true);

		/**
		 * \brief Coasts the robot’s wheels
		 */
		void drive_coast();

		/**
		 * \brief Brakes the robot’s wheels
		 */
		void drive_brake();

		/**
		 * \brief Turns the dribbler motor on or off
		 *
		 * \param[in] active \c true to turn the motor on, or \c false to turn it off
		 */
		void dribble(bool active = true);

		/**
		 * \brief Sets the state of the capacitor charger
		 *
		 * \param[in] state the state to set the charger to
		 */
		void set_charger_state(ChargerState state);

		/**
		 * \brief Executes a kick
		 *
		 * \param[in] chip \c true to fire the chipper, or \c false to fire the kicker
		 *
		 * \param[in] pulse_width the width of the pulse to send to the solenoid, in quarters of a microsecond
		 */
		void kick(bool chip, unsigned int pulse_width);

		/**
		 * \brief Enables or disables automatic kicking when the ball breaks the robot’s laser
		 *
		 * If the pulse width is set to zero, automatic kicking will be disabled
		 *
		 * \param[in] chip \c true to fire the chipper, or \c false to fire the kicker
		 *
		 * \param[in] pulse_width the width of the pulse to send to the solenoid, in quarters of a microsecond
		 */
		void autokick(bool chip, unsigned int pulse_width);

	private:
		friend class MRFDongle;

		MRFDongle &dongle;
		Annunciator::Message hall_stuck_message, charge_timeout_message;

		MRFRobot(MRFDongle &dongle, unsigned int index);
		void handle_message(const void *data, std::size_t len);
};

#endif

