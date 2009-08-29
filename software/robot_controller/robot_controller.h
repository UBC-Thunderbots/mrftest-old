#ifndef ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_H

#include <glibmm.h>
#include "geom/point.h"
#include "util/noncopyable.h"
#include "world/player.h"

//
// Translates world-coordinate movement requests into robot-relative
// velocities.
//
class robot_controller : public noncopyable {
	public:
		//
		// A pointer to a robot_controller.
		//
		typedef Glib::RefPtr<robot_controller> ptr;

		//
		// Constructs a new robot_controller for the specified robot.
		//
		robot_controller(player::ptr bot);

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
		void move(const point &position, double orientation) = 0;
};

#endif

