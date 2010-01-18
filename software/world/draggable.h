#ifndef WORLD_DRAGGABLE_H
#define WORLD_DRAGGABLE_H

#include "geom/point.h"
#include "util/byref.h"

//
// An object that can be dragged around in the visualizer and manipulated by
// framework code such as autorefs.
//
class draggable : public byref {
	public:
		//
		// A pointer to a draggable.
		//
		typedef Glib::RefPtr<draggable> ptr;

		//
		// Gets the position of the object.
		//
		virtual point position() const __attribute__((warn_unused_result)) = 0;

		//
		// Sets the position and velocity of the object. This should NOT be
		// called from the AI!
		//
		virtual void ext_drag(const point &pos, const point &vel) = 0;

		//
		// Sets the orientation and angular velocity of the object. This should
		// NOT be called from the AI!
		//
		virtual void ext_rotate(double orient, double avel) = 0;
};

#endif

