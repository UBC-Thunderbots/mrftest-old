#ifndef FIELD_H
#define FIELD_H

#include "geom/point.h"
#include "util/noncopyable.h"

//
// The playing field. All lengths are in metres.
//
class field : public noncopyable {
	public:
		//
		// The length of the field, from goal to goal, in the X direction.
		//
		double length() const;

		//
		// The width of the field, from sideline to sideline, in the Y
		// direction.
		//
		double width() const;

		//
		// The width of the goal, symmetric above and below the centreline.
		//
		double goal_width() const;

		//
		// The radius of the centre circle.
		//
		double centre_circle_radius() const;

		//
		// The radius of the arcs at the top and bottom of the defense area.
		//
		double defense_area_radius() const;

		//
		// The width of the straight part between the arcs in the defense area.
		//
		double defense_area_stretch() const;
};

#endif

