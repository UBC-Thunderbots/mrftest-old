#ifndef WORLD_DRAGGABLE_H
#define WORLD_DRAGGABLE_H

#include "geom/point.h"

//
// An object that can be dragged around in the visualizer.
//
class draggable {
	public:
		//
		// Sets the position of the object. This should ONLY be called from the
		// UI, not from the AI!
		//
		virtual void ui_set_position(const point &) = 0;
};

#endif

