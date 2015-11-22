#ifndef DRIVE_ROBOT_H
#define DRIVE_ROBOT_H

#include "geom/point.h"
#include "util/annunciator.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <cstdint>
#include <functional>

namespace Drive {
	class Dongle;

	/**
	 * \brief The movement primitive codes.
	 */
	enum class Primitive {
		/**
		 * \brief Implements the \ref Drive::Robot::move_coast and \ref
		 * Drive::Robot::move_brake primitives.
		 *
		 * The parameters are unused.
		 *
		 * The extra field is 0 for coasting or 1 for braking.
		 */
		STOP,

		/**
		 * \brief Implements the \ref Drive::Robot::move_move family of
		 * primitives.
		 *
		 * The parameters are the relative position, the relative orientation,
		 * and the time delta.
		 *
		 * The extra field is 0 if the caller doesn’t care about orientation,
		 * or 1 if it does.
		 */
		MOVE,

		/**
		 * \brief Implements the \ref Drive::Robot::move_dribble primitive.
		 *
		 * The parameters are the relative position and orientation.
		 *
		 * The extra field is 0 if small kicks are prohibited or 1 if they are
		 * allowed.
		 */
		DRIBBLE,

		/**
		 * \brief Implements the \ref Drive::Robot::move_shoot family of
		 * primitives.
		 *
		 * The parameters are the relative position, relative orientation, and
		 * power (either m/s or m).
		 *
		 * The extra field has bit 0 clear to kick or set to chip, and bit 1
		 * set if the caller cares about orientation.
		 */
		SHOOT,

		/**
		 * \brief Implements the \ref Drive::Robot::move_catch primitive.
		 *
		 * The parameters are the angle difference, the left/right
		 * displacement, and the speed.
		 */
		CATCH,

		/**
		 * \brief Implements the \ref Drive::Robot::move_pivot primitive.
		 *
		 * The parameters are the relative centre point, the swing, and the
		 * orientation.
		 */
		PIVOT,

		/**
		 * \brief Implements the \ref Drive::Robot::move_spin primitive.
		 *
		 * The parameters are the relative position and angular velocity.
		 */
		SPIN,

		/**
		 * \brief Specifies that direct control is in use and wheels are being
		 * driven with individual power levels.
		 */
		DIRECT_WHEELS,

		/**
		 * \brief Specifies that direct control is in use and robot-relative
		 * linear and angular velocities are being sent.
		 */
		DIRECT_VELOCITY,
	};

	/**
	 * \brief A generic driveable robot.
	 */
	class Robot : public NonCopyable {
		public:
			/**
			 * \brief The possible states of the charger.
			 */
			enum class ChargerState {
				/**
				 * \brief Safely discharges the capacitor down to battery level.
				 */
				DISCHARGE,

				/**
				 * \brief Neither charges nor discharges the capacitors.
				 */
				FLOAT,

				/**
				 * \brief Charges the capacitor to full voltage.
				 */
				CHARGE,
			};

			/**
			 * \brief The pattern index of the robot.
			 */
			const unsigned int index;

			/**
			 * \brief The rough maximum full-scale deflection of the laser
			 * sensor.
			 */
			const double break_beam_scale;

			/**
			 * \brief The maximum possible kick speed, in m/s.
			 */
			const double kick_speed_max;
	
			/**
			 * \brief The kick resolution for HScale.
			 */

			const double kick_speed_resolution;

			/**
			 * \brief The maximum possible chip distance, in m.
			 */
			const double chip_distance_max;

			/**
			 * \brief The chip resolution for HScale.
			 */		

			const double chip_distance_resolution;

			/**
			 * \brief The maximum power level understood by the \ref
			 * direct_dribbler function.
			 */
			const unsigned int direct_dribbler_max;

			/**
			 * \brief Whether or not the robot is currently responding to radio
			 * communication.
			 */
			Property<bool> alive;

			/**
			 * \brief Whether the robot is in direct mode.
			 */
			Property<bool> direct_control;

			/**
			 * \brief Whether or not the ball is interrupting the robot’s laser
			 * beam.
			 */
			Property<bool> ball_in_beam;

			/**
			 * \brief Whether or not the robot’s capacitor is charged enough to
			 * kick the ball.
			 */
			Property<bool> capacitor_charged;

			/**
			 * \brief The voltage on the robot’s battery, in volts.
			 */
			Property<double> battery_voltage;

			/**
			 * \brief The voltage on the robot’s kicking capacitor, in volts.
			 */
			Property<double> capacitor_voltage;

			/**
			 * \brief The reading of the robot’s laser sensor.
			 */
			Property<double> break_beam_reading;

			/**
			 * \brief The temperature of the robot’s dribbler motor, in degrees
			 * Celsius.
			 */
			Property<double> dribbler_temperature;

			/**
			 * \brief The speed of the robot’s dribbler motor, in revolutions
			 * per minute.
			 */
			Property<int> dribbler_speed;

			/**
			 * \brief The temperature of the robot’s mainboard, in degrees
			 * Celsius.
			 */
			Property<double> board_temperature;

			/**
			 * \brief The LPS reflectance values.
			 */
			std::array<Property<double>, 4> lps_values;

			/**
			 * \brief The link quality of the last received packet, from 0 to
			 * 1.
			 */
			Property<double> link_quality;

			/**
			 * \brief The received signal strength of the last received packet,
			 * in decibels.
			 */
			Property<int> received_signal_strength;

			/**
			 * \brief Whether or not the build ID information is valid.
			 */
			Property<bool> build_ids_valid;

			/**
			 * \brief The microcontroller firmware build ID.
			 */
			Property<uint32_t> fw_build_id;

			/**
			 * \brief The FPGA bitstream build ID.
			 */
			Property<uint32_t> fpga_build_id;

			/**
			 * \brief The current executing primitive.
			 */
			Property<Primitive> primitive;

			/**
			 * \brief Emitted when the autokick mechanism causes the robot to
			 * kick.
			 */
			sigc::signal<void> signal_autokick_fired;

			/**
			 * \name Miscellaneous Functions
			 * \{
			 */

			/**
			 * \brief Destroys a Robot.
			 */
			virtual ~Robot();

			/**
			 * \brief Returns the dongle controlling the robot.
			 *
			 * \return the dongle
			 */
			virtual Drive::Dongle &dongle() = 0;

			/**
			 * \brief Returns the dongle controlling the robot.
			 *
			 * \return the dongle
			 */
			virtual const Drive::Dongle &dongle() const = 0;

			/**
			 * \brief Sets the state of the capacitor charger.
			 *
			 * \param[in] state the state to set the charger to
			 */
			virtual void set_charger_state(ChargerState state) = 0;

			/**
			 * \}
			 */

			/**
			 * \name Movement Primitives
			 *
			 * The functions in this group can only be invoked if direct
			 * control is inactive. Direct control is inactive by default or
			 * after \ref direct_control is set to false.
			 *
			 * \{
			 */

			/**
			 * \brief Sets whether the robot is limited to moving slowly.
			 *
			 * This function can be used to enforce rules about robot movement
			 * speed due to certain play types.
			 *
			 * \param[in] slow \c true to move slowly, or \c false to move fast
			 */
			virtual void move_slow(bool slow = true) = 0;

			/**
			 * \brief Coasts the robot’s wheels.
			 */
			virtual void move_coast() = 0;

			/**
			 * \brief Brakes the robot’s wheels.
			 */
			virtual void move_brake() = 0;

			/**
			 * \brief Moves the robot to a target position.
			 *
			 * The robot’s orientation upon reaching the target position is
			 * unspecified.
			 *
			 * \param[in] dest the position to move to, as a distance forward
			 * and left of the robot’s current position, relative to the
			 * robot’s current orientation
			 */
			virtual void move_move(Point dest) = 0;

			/**
			 * \brief Moves the robot to a target position and orientation.
			 *
			 * \param[in] dest the position to move to, as a distance forward
			 * and left of the robot’s current position, relative to the
			 * robot’s current orientation
			 * \param[in] orientation how far left to rotate the robot to reach
			 * its desired orientation
			 */
			virtual void move_move(Point dest, Angle orientation) = 0;

			/**
			 * \brief Moves the robot to a target position.
			 *
			 * The robot’s orientation upon reaching the target position is
			 * unspecified.
			 *
			 * \param[in] dest the position to move to, as a distance forward
			 * and left of the robot’s current position, relative to the
			 * robot’s current orientation
			 * \param[in] time_delta how many seconds in the future the robot
			 * should arrive at its destination
			 */
			virtual void move_move(Point dest, double time_delta) = 0;

			/**
			 * \brief Moves the robot to a target position and orientation.
			 *
			 * \param[in] dest the position to move to, as a distance forward
			 * and left of the robot’s current position, relative to the
			 * robot’s current orientation
			 * \param[in] orientation how far left to rotate the robot to reach
			 * its desired orientation
			 * \param[in] time_delta how many seconds in the future the robot
			 * should arrive at its destination
			 */
			virtual void move_move(Point dest, Angle orientation, double time_delta) = 0;

			/**
			 * \brief Moves the robot while carrying the ball.
			 *
			 * \param[in] dest the position to move to, as a distance forward
			 * and left of the robot’s current position, relative to the
			 * robot’s current orientation
			 * \param[in] orientation how far left to rotate the robot to reach
			 * its desired orientation
			 * \param[in] small_kick_allowed whether or not the robot is
			 * allowed to kick the ball ahead of itself while moving
			 */
			virtual void move_dribble(Point dest, Angle orientation, bool small_kick_allowed) = 0;

			/**
			 * \brief Kicks the ball.
			 *
			 * The direction of the kick, and the robot’s final orientation,
			 * are unspecified.
			 *
			 * \param[in] dest the position of the ball, as a distance forward
			 * and left of the robot’s current position, relative to the
			 * robot’s current orientation
			 * \param[in] power how fast in m/s (for kicking) or how far in m
			 * (for chipping) to kick the ball
			 * \param[in] chip \c true to chip the ball or \c false to kick it
			 */
			virtual void move_shoot(Point dest, double power, bool chip) = 0;

			/**
			 * \brief Kicks the ball.
			 *
			 * \param[in] dest the position of the ball, as a distance forward
			 * and left of the robot’s current position, relative to the
			 * robot’s current orientation
			 * \param[in] orientation how far left the robot should rotate
			 * before it kicks in order to kick the ball in the desired
			 * direction
			 * \param[in] power how fast in m/s (for kicking) or how far in m
			 * (for chipping) to kick the ball
			 * \param[in] chip \c true to chip the ball or \c false to kick it
			 */
			virtual void move_shoot(Point dest, Angle orientation, double power, bool chip) = 0;

			/**
			 * \brief Catches a moving ball.
			 *
			 * \param[in] angle_diff how far left of the robot’s current
			 * orientation would make it be pointing exactly 180° from the
			 * ball’s current velocity (and thus pointing in the perfect
			 * direction to receive the ball)
			 * \param[in] displacement the distance to the left of the robot’s
			 * current position to move to in order to be in the ball’s path,
			 * where “left” is defined relative to the robot’s orientation
			 * <em>after the requested rotation</em>
			 * \param[in] speed the speed the robot should move forward (or
			 * negative for backward), where “forward” is defined relative to
			 * the robot’s orientation <em>after the requested rotation</em>
			 */
			virtual void move_catch(Angle angle_diff, double displacement, double speed) = 0;

			/**
			 * \brief Rotates around a point on the field (e.g. the ball) while
			 * facing it.
			 *
			 * Throughout the pivot, the robot maintains a fixed distance from
			 * the centre point.
			 *
			 * \param[in] centre the position of the centre of the circle, as a
			 * distance forward and left of the robot’s current position,
			 * relative to the robot’s current orientation
			 * \param[in] swing how far counterclockwise to swing the vector
			 * from centre point to robot about the centre point to get the
			 * final position of the robot
			 * \param[in] orientation how far left the robot should rotate to
			 * reach the desired final orientation
			 */
			virtual void move_pivot(Point centre, Angle swing, Angle orientation) = 0;

			/**
			 * \brief Spins around rapidly while moving.
			 *
			 * \param[in] dest the position to move to, as a distance forward
			 * and left of the robot’s current position, relative to the
			 * robot’s current orientation
			 * \param[in] speed the speed to spin at, in units per second
			 */
			virtual void move_spin(Point dest, Angle speed) = 0;

			/**
			 * \}
			 */

			/**
			 * \name Direct Hardware Control
			 *
			 * The functions in this group can only be invoked if direct
			 * control is active. Direct control is active after \ref
			 * direct_control is set to \c true.
			 *
			 * \{
			 */

			/**
			 * \brief Sets the raw power levels sent to the robot’s wheels.
			 *
			 * The power levels are sent directly to the motors without any
			 * control.
			 *
			 * \param[in] wheels the power levels sent to the wheels, in the
			 * range ±255
			 */
			virtual void direct_wheels(const int (&wheels)[4]) = 0;

			/**
			 * \brief Sets a relative linear and angular velocity for the robot
			 * to drive at.
			 *
			 * \param[in] vel the relative linear velocity at which to drive
			 * \param[in] avel the angular velocity at which to drive
			 */
			virtual void direct_velocity(Point vel, Angle avel) = 0;

			/**
			 * \brief Controls the dribbler motor.
			 *
			 * \param[in] power the power level to use, with 0 meaning stop and
			 * a maximum value given by \ref direct_dribbler_max
			 */
			virtual void direct_dribbler(unsigned int power) = 0;

			/**
			 * \brief Fires the chicker.
			 *
			 * \param[in] power the power, in metres per second speed (for a
			 * kick) or metres distance (for a chip)
			 * \param[in] chip \c true to fire the chipper or \c false to fire
			 * the kicker
			 */
			virtual void direct_chicker(double power, bool chip) = 0;

			/**
			 * \brief Enables or disables automatic kicking when the ball
			 * breaks the robot’s laser.
			 *
			 * If \p power is zero, automatic kicking will be disabled.
			 *
			 * \param[in] power the power, in metres per second speed (for a
			 * kick) or metres distance (for a chip)
			 * \param[in] chip \c true to fire the chipper, or \c false to fire
			 * the kicker
			 */
			virtual void direct_chicker_auto(double power, bool chip) = 0;


			/**
			 * \}
			 */

		protected:
			/**
			 * \brief Constructs a new Robot.
			 *
			 * \param[in] index the pattern index of the robot
			 * \param[in] break_beam_scale the rough maximum full-scale
			 * deflection of the laser sensor
			 * \param[in] kick_speed_max the maximum possible kick speed, in
			 * m/s
			 * \param[in] chip_distance_max the maximum possible chip distance,
			 * in m
			 * \param[in] direct_dribbler_max the maximum power level
			 * understood by the \ref direct_dribbler function
			 */
			explicit Robot(unsigned int index, double break_beam_scale, double kick_speed_max, double chip_distance_max, unsigned int direct_dribbler_max);

		private:
			Annunciator::Message low_battery_message, high_board_temperature_message;

			void handle_alive_changed();
			void handle_battery_voltage_changed();
			void handle_board_temperature_changed();
	};
}

namespace std {
	template<> struct hash<Drive::Primitive> {
		std::size_t operator()(Drive::Primitive x) const {
			return static_cast<std::size_t>(x);
		}
	};
}

#endif
