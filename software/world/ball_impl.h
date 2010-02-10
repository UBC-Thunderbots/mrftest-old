#ifndef WORLD_BALL_IMPL_H
#define WORLD_BALL_IMPL_H

#include "world/draggable.h"
#include "world/predictable.h"

//
// The ball, as provided by the world. An implementation of the world must
// provide an implementation of this class and use it to construct ball objects
// which can be given to the AI. Vectors in this class are in global
// coordinates.
//
class ball_impl : public predictable, public draggable {
	public:
		//
		// A pointer to a ball_impl.
		//
		typedef Glib::RefPtr<ball_impl> ptr;

		//
		// The position of the ball at the last camera frame.
		//
		virtual point position() const __attribute__((warn_unused_result)) = 0;

		//
		// It does not make sense to deal with the "orientation" of a sphere.
		// To eliminate the need for all subclasses to pointlessly implement
		// this function, it is implemented here and does nothing.
		//
		void ext_rotate(double, double) {
		}

		//
		// Returns a trivial implementation of ball_impl that always leaves the
		// ball sitting at the origin.
		//
		static ptr trivial();
};

#endif

