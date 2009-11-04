#ifndef WORLD_DRAGGABLE_H
#define WORLD_DRAGGABLE_H

#include "geom/point.h"
#include "util/byref.h"
#include <glibmm.h>

//
// An object that can be dragged around in the visualizer.
//
class draggable : public virtual byref {
	public:
		//
		// A pointer to a draggable.
		//
		typedef Glib::RefPtr<draggable> ptr;

		//
		// Sets the position of the object. This should ONLY be called from the
		// UI, not from the AI!
		//
		virtual void ui_set_position(const point &) = 0;
};

#endif

