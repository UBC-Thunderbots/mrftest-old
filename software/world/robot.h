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
		// An ID number that uniquely identifies the robot across all teams.
		//
		unsigned int id() const {
			return the_id;
		}

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
		// Constructs a new robot object.
		//
		// Parameters:
		//  id
		//   the global ID number of this robot
		//
		//  impl
		//   the implementation object that provides global coordinates
		//
		//  flip
		//   whether the X and Y coordinates are reversed for this object
		//
		robot(unsigned int id, robot_impl::ptr impl, bool flip) : the_id(id), impl(impl), flip(flip) {
		}

	private:
		const unsigned int the_id;
		robot_impl::ptr impl;
		const bool flip;
};

#endif

