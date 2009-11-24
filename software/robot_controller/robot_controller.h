#ifndef ROBOT_CONTROLLER_ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_ROBOT_CONTROLLER_H

#include "geom/point.h"
#include "util/byref.h"

class player_impl;
class robot_controller_factory;

//
// Translates world-coordinate movement requests into robot-relative
// velocities.
//
class robot_controller : public byref {
	public:
		//
		// A pointer to a robot_controller.
		//
		typedef Glib::RefPtr<robot_controller> ptr;

		//
		// Tells the robot controlled by this controller to move to the
		// specified target location and orientation.
		//
		// Parameters:
		//  new_position
		//   the position to move to, in world coordinates measured in metres
		//
		//  new_orientation
		//   the orientation to rotate to in world coordinates measured in
		//   radians
		//
		//  linear_velocity
		//   (output) the linear velocity to move at, in robot-relative
		//   coordinates robot-relative coordinates are defined as the positive
		//   X axis being forward and the positive Y axis being left
		//
		//  angular_velocity
		//   (output) the angular velocity to move at
		//
		// It is expected that this function will update internal state. It is
		// guaranteed that this function will be called exactly once per timer
		// tick.
		//
		virtual void move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) = 0;

		//
		// Returns the factory that created this controller.
		//
		virtual robot_controller_factory &get_factory() const = 0;

	protected:
		//
		// Constructs a new robot_controller.
		//
		robot_controller() {
		}
};

//
// A factory to construct robot_controllers.
// 
class robot_controller_factory : public noncopyable {
	public:
		//
		// The type of the map returned by the all() method.
		//
		typedef std::map<Glib::ustring, robot_controller_factory *> map_type;

		//
		// The name of the robot controllers created by this factory.
		//
		const Glib::ustring &name() const {
			return the_name;
		}

		//
		// Constructs a new robot_controller.
		//
		// Virtually all controller implementations should ignore the "yellow"
		// parameter. It is intended for use only by controllers doing VERY,
		// VERY special things.
		//
		// The controller can keep a reference to the player_impl object if it
		// wishes. It MUST not call any mutating methods on the player_impl;
		// however, it may call into the player_impl to retrieve the player's
		// current position, orientation, estimated velocity, and so on.
		//
		virtual robot_controller::ptr create_controller(Glib::RefPtr<player_impl> plr, bool yellow, unsigned int index) = 0;

		//
		// Gets the collection of all registered controller factories, keyed by name.
		//
		static const map_type &all();

	protected:
		//
		// Constructs a robot_controller_factory.
		//
		robot_controller_factory(const Glib::ustring &name);

		//
		// Destroys a robot_controller_factory.
		//
		virtual ~robot_controller_factory();

	private:
		Glib::ustring the_name;
};

#endif

