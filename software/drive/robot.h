#ifndef DRIVE_ROBOT_H
#define DRIVE_ROBOT_H

#include "util/noncopyable.h"
#include "util/property.h"

namespace Drive {
	/**
	 * \brief A generic driveable robot
	 */
	class Robot : public NonCopyable {
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
			 * \brief The pattern index of the robot
			 */
			const unsigned int index;

			/**
			 * \brief Whether or not the robot is currently responding to radio communication
			 */
			Property<bool> alive;

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
			Property<int> break_beam_reading;

			/**
			 * \brief The temperature of the robot’s dribbler motor, in degrees Celsius
			 */
			Property<double> dribbler_temperature;

			/**
			 * \brief The temperature of the robot’s mainboard, in degrees Celsius
			 */
			Property<double> board_temperature;

			/**
			 * \brief Emitted when the autokick mechanism causes the robot to kick
			 */
			sigc::signal<void> signal_autokick_fired;

			/**
			 * \brief Destroys a \code Robot
			 */
			virtual ~Robot();

			/**
			 * \brief Sets the speeds of the robot’s wheels
			 *
			 * \param[in] wheels the speeds of the wheels, in quarters of a degree per five milliseconds, in the range ±1023
			 *
			 * \param[in] controlled \c true to run the provided setpoints through the wheel controllers, or \c false to run open-loop
			 */
			virtual void drive(const int(&wheels)[4], bool controlled = true) = 0;

			/**
			 * \brief Checks whether the robot can actually coast its wheels
			 *
			 * \return \c true if the robot can coast its wheels, or \c false if it can only brake
			 */
			virtual bool can_coast() const = 0;

			/**
			 * \brief Coasts the robot’s wheels
			 *
			 * If the robot cannot coast its wheels (as indicated by \ref can_coast() returning \c false), it brakes instead.
			 *
			 * \param[in] wheels the PWM levels of the wheels, in case manual commutation is active
			 */
			virtual void drive_coast_or_manual(const int(&wheels)[4]) = 0;

			/**
			 * \brief Brakes the robot’s wheels
			 */
			virtual void drive_brake() = 0;

			/**
			 * \brief Turns the dribbler motor on or off
			 *
			 * \param[in] active \c true to turn the motor on, or \c false to turn it off
			 */
			virtual void dribble(bool active = true) = 0;

			/**
			 * \brief Sets the state of the capacitor charger
			 *
			 * \param[in] state the state to set the charger to
			 */
			virtual void set_charger_state(ChargerState state) = 0;

			/**
			 * \brief Indicates the maximum kicker pulse width the robot is capable of executing
			 *
			 * \return the maximum pulse width, in microseconds
			 */
			virtual double kick_pulse_maximum() const = 0;

			/**
			 * \brief Indicates the resolution of kicker pulse width the robot is capable of encoding
			 *
			 * \return the pulse width resolution, in microseconds
			 */
			virtual double kick_pulse_resolution() const = 0;

			/**
			 * \brief Executes a kick
			 *
			 * \param[in] chip \c true to fire the chipper, or \c false to fire the kicker
			 *
			 * \param[in] pulse_width the width of the pulse to send to the solenoid, in microseconds
			 */
			virtual void kick(bool chip, double pulse_width) = 0;

			/**
			 * \brief Enables or disables automatic kicking when the ball breaks the robot’s laser
			 *
			 * If the pulse width is set to zero, automatic kicking will be disabled
			 *
			 * \param[in] chip \c true to fire the chipper, or \c false to fire the kicker
			 *
			 * \param[in] pulse_width the width of the pulse to send to the solenoid, in microseconds
			 */
			virtual void autokick(bool chip, double pulse_width) = 0;

		protected:
			/**
			 * \brief Constructs a new \code Robot
			 *
			 * \param[in] index the pattern index of the robot
			 */
			Robot(unsigned int index);
	};
}

#endif

