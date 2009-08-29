#ifndef ROBOT_H
#define ROBOT_H

#include <glibmm.h>
#include "geom/point.h"
#include "util/noncopyable.h"

class robot : public noncopyable {
	public:
		//
		// A pointer to a robot object.
		//
		typedef Glib::RefPtr<robot> ptr;

		//
		// The position of the robot at the last camera frame.
		//
		const point &position() const = 0;

		//
		// The orientation of the robot in radians at the last camera frame.
		//
		double orientation() const = 0;
};

#endif

