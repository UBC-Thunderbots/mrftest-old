#ifndef WORLD_BALL_IMPL_H
#define WORLD_BALL_IMPL_H

#include "geom/point.h"
#include "util/byref.h"
#include "world/predictable.h"
#include <glibmm/refptr.h>

//
// The ball, as provided by the world. An implementation of the world must
// provide an implementation of this class and use it to construct ball objects
// which can be given to the AI. Vectors in this class are in global
// coordinates.
//
class ball_impl : public virtual byref, public virtual predictable {
	public:
		//
		// A pointer to a ball_impl.
		//
		typedef Glib::RefPtr<ball_impl> ptr;

		//
		// The position of the ball at the last camera frame.
		//
		virtual point position() const = 0;

		//
		// The estimated velocity of the ball at the last camera frame.
		//
		virtual point velocity() const = 0;

		//
		// The estimated acceleration of the ball at the last camera frame.
		// 
		virtual point acceleration() const = 0;

		//
		// Returns a trivial implementation of ball_impl that always leaves the
		// ball sitting at the origin.
		//
		static const ptr &trivial();
};

#endif

