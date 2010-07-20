#ifndef ROBOT_CONTROLLER_ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_ROBOT_CONTROLLER_H

#include "geom/point.h"
#include "util/memory.h"
#include "util/registerable.h"

class Player;
class RobotControllerFactory;

/**
 * Translates world-coordinate movement requests into robot wheel rotation
 * speeds.
 */
class RobotController2 : public ByRef {
	public:
		/**
		 * Tells the robot controlled by this controller to move to the
		 * specified target location and orientation.
		 *
		 * It is expected that this function will update internal state. It is
		 * guaranteed that this function will be called exactly once per timer
		 * tick, except for those ticks in which clear() is called instead.
		 *
		 * \param new_position the position to move to, in team coordinates
		 * measured in metres.
		 *
		 * \param new_orientation the orientation to rotate to in team
		 * coordinates measured in radians.
		 *
		 * \param wheel_speeds (output) the speeds of the four wheels to send to
		 * the robot, in quarters of a degree of motor shaft rotation per five
		 * milliseconds.
		 */
		virtual void move(const Point &new_position, double new_orientation, int (&wheel_speeds)[4]) = 0;

		/**
		 * Tells the controller to clear its internal state because the robot
		 * under control is scrammed. The controller should clear any
		 * integrators and similar structures in order to prevent unexpected
		 * jumps when driving resumes.
		 */
		virtual void clear() = 0;

		/**
		 * \return the factory that created this controller.
		 */
		virtual RobotControllerFactory &get_factory() const = 0;

		/**
		 * Multiplies a robot-relative velocity tuple by the wheel matrix to
		 * produce a set of wheel rotation speeds. A robot controller
		 * implementation may choose to take advantage of this function, or may
		 * choose to complete ignore it and compute wheel speeds in a different
		 * way.
		 *
		 * \param[in] vel the linear velocity to multiply, in metres per second.
		 *
		 * \param[in] avel the angular velocity to multiply, in radians per
		 * second.
		 *
		 * \param[out] wheel_speeds the wheel speeds, in quarters of a degree of
		 * motor shaft rotation per five milliseconds.
		 */
		static void convert_to_wheels(const Point &vel, double avel, int (&wheel_speeds)[4]);

	protected:
		/**
		 * Constructs a new RobotController.
		 */
		RobotController2() {
		}
};

/**
 * A compatibility layer for using old robot controllers that prefer to produce
 * output in the form of linear and angular velocities in robot-relative metres
 * per second, rather than calculating wheel speeds directly.
 */
class RobotController : public RobotController2 {
	public:
		/**
		 * Tells the robot controlled by this controller to move to the
		 * specified target location and orientation.
		 *
		 * It is expected that this function will update internal state. It is
		 * guaranteed that this function will be called exactly once per timer
		 * tick.
		 *
		 * \param new_position the position to move to, in team coordinates
		 * measured in metres
		 * \param new_orientation the orientation to rotate to in team
		 * coordinates measured in radians
		 * \param linear_velocity (output) the linear velocity to move at, in
		 * robot-relative coordinates (defined as the positive X axis being
		 * forward and the positive Y axis being left)
		 * \param angular_velocity (output) the angular velocity to rotate at,
		 * with positive being to the left
		 */
		virtual void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity) = 0;

	private:
		void move(const Point &new_position, double new_orientation, int (&wheel_speeds)[4]);
};

/**
 * A factory to construct robot_controllers.
 */ 
class RobotControllerFactory : public Registerable<RobotControllerFactory> {
	public:
		/**
		 * Constructs a new RobotController.
		 *
		 * \param plr the robot being controlled (a reference should be kept;
		 * the controller must call only inspection methods on the object as
		 * mutation methods are intended to be called from the AI)
		 * \param yellow the colour of the robot (should generally be ignored;
		 * intended to be used only in VERY, VERY special situations)
		 * \param index the pattern index of the robot (should generally be
		 * ignored; intended to be used only in VERY, VERY special situations)
		 * \return The new controller
		 */
		virtual RefPtr<RobotController2> create_controller(Glib::RefPtr<Player> plr, bool yellow, unsigned int index) const = 0;

	protected:
		/**
		 * Constructs a RobotControllerFactory. This is intended to be called
		 * from a subclass constructor as a result of a global variable holding
		 * an instance of the subclass coming into scope at application startup.
		 *
		 * \param name a human-readable name for the factory
		 */
		RobotControllerFactory(const Glib::ustring &name) : Registerable<RobotControllerFactory>(name) {
		}
};

#endif

