#ifndef WORLD_FIELD_H
#define WORLD_FIELD_H

#include <glibmm/refptr.h>
#include "util/byref.h"

//
// The playing field. All lengths are in metres.
// The origin of this coordinate system is the centre of the field.
// The positive x direction is the direction to the enemy goal.
//
class field : public virtual byref {
	public:
		//
		// A pointer to a field object.
		//
		typedef Glib::RefPtr<field> ptr;

		//
		// The length of the field, from goal to goal, in the X direction.
		//
		virtual double length() const = 0;

		//
		// The width of the field, from sideline to sideline, in the Y
		// direction.
		//
		virtual double width() const = 0;

		//
		// The width of the goal, symmetric above and below the centreline.
		//
		virtual double goal_width() const = 0;

		//
		// The radius of the centre circle.
		//
		virtual double centre_circle_radius() const = 0;

		//
		// The radius of the arcs at the top and bottom of the defense area.
		//
		virtual double defense_area_radius() const = 0;

		//
		// The width of the straight part between the arcs in the defense area.
		//
		virtual double defense_area_stretch() const = 0;
};

#endif

