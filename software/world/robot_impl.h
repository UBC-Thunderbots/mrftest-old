#ifndef WORLD_ROBOT_IMPL_H
#define WORLD_ROBOT_IMPL_H

#include "world/draggable.h"
#include "world/predictable.h"

//
// A robot can be either friendly or enemy. An implementation of the world must
// provide an implementation of this class and use it to construct robot objects
// to pass to the AI. Vectors in this class are in global coordinates.
//
class robot_impl : public predictable, public draggable {
	public:
		//
		// A pointer to a robot_impl.
		//
		typedef Glib::RefPtr<robot_impl> ptr;

		//
		// The position of the robot at the last camera frame.
		//
		virtual point position() const = 0;

		//
		// The orientation of the robot in radians at the last camera frame.
		//
		virtual double orientation() const = 0;
};

#endif

