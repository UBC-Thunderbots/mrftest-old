#ifndef WORLD_ROBOT_H
#define WORLD_ROBOT_H

#include <glibmm/refptr.h>
#include "geom/angle.h"
#include "geom/point.h"
#include "util/byref.h"
#include "world/robot_impl.h"

//
// A robot can be either friendly or enemy. Vectors in this class are in team
// coordinates.
//
class robot : public virtual byref {
	public:
		//
		// A pointer to a robot object.
		//
		typedef Glib::RefPtr<robot> ptr;

		//
		// The position of the robot at the last camera frame.
		//
		point position() const {
			return impl->position() * (flip ? -1.0 : 1.0);
		}

		//
		// The orientation of the robot in radians at the last camera frame.
		//
		double orientation() const {
			return angle_mod(impl->orientation() + (flip ? PI : 0.0));
		}

		//
		// Whether or not this robot has possession of the ball.
		//
		bool has_ball() const {
			return impl->has_ball();
		}

		//
		// Constructs a new robot object.
		//
		// Parameters:
		//  impl
		//   the implementation object that provides global coordinates
		//
		//  flip
		//   whether the X and Y coordinates are reversed for this object
		//
		robot(robot_impl::ptr impl, bool flip) : impl(impl), flip(flip) {
		}

	private:
		robot_impl::ptr impl;
		const bool flip;
};

#endif

