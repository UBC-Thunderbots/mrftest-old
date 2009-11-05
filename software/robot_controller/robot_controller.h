#ifndef ROBOT_CONTROLLER_ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_ROBOT_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"

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
		//  current_position
		//   the current position of the robot
		//
		//  new_position
		//   the position to move to, in world coordinates measured in metres
		//
		//  current_orientation
		//   the current orientation of the robot
		//
		//  new_orientation
		//   the orientation to rotate to in world coordinates measured in
		//   radians
		//
		//  linear_velocity
		//   (output) the linear velocity to move at, in robot-relative coordinates
		//   robot-relative coordinates are defined as the positive X axis being forward
		//   and the positive Y axis being left
		//
		//  angular_velocity
		//   (output) the angular velocity to move at
		//
		virtual void move(const point &current_position, const point &new_position, double current_orientation, double new_orientation, point &linear_velocity, double &angular_velocity) = 0;

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
		virtual robot_controller::ptr create_controller(const Glib::ustring &robot_name) = 0;

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

