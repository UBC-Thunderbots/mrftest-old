#ifndef ROBOT_CONTROLLER_ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_ROBOT_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "geom/point.h"
#include "util/byref.h"
#include "world/player.h"

//
// Translates world-coordinate movement requests into robot-relative
// velocities.
//
class robot_controller : public virtual byref {
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
		//  position
		//   the position to move to, in world coordinates measured in metres
		//
		//  orientation:
		//   the orientation to rotate to in world coordinates measured in
		//   radians
		//
		virtual void move(const point &position, double orientation) = 0;

	protected:
		//
		// Constructs a new robot_controller for the specified robot.
		//
		robot_controller(player::ptr bot) : robot(bot) {
		}

		//
		// The robot controlled by this controller.
		//
		const player::ptr robot;
};

//
// A factory to construct robot_controllers.
// 
class robot_controller_factory : public virtual byref {
	public:
		//
		// The name of the robot controllers created by this factory.
		//
		const Glib::ustring &name() const {
			return the_name;
		}

		//
		// Constructs a new robot_controller.
		//
		virtual robot_controller::ptr create_controller(player::ptr bot) = 0;

		//
		// Gets the collection of all registered controllers, keyed by name.
		//
		static const std::map<Glib::ustring, robot_controller_factory *> &all();

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

