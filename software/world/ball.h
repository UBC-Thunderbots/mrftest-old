#ifndef WORLD_BALL_H
#define WORLD_BALL_H

#include <glibmm/refptr.h>
#include "geom/point.h"
#include "util/byref.h"
#include "world/ball_impl.h"

//
// The ball, as seen by the AI. Vectors in this class are in team coordinates.
//
class ball : public virtual byref {
	public:
		//
		// A pointer to a ball object.
		//
		typedef Glib::RefPtr<ball> ptr;

		//
		// The position of the ball at the last camera frame.
		//
		point position() const {
			return impl.position() * (flip ? -1.0 : 1.0);
		}

		//
		// The estimated velocity of the ball at the last camera frame.
		//
		point velocity() const {
			return impl.velocity() * (flip ? -1.0 : 1.0);
		}

		//
		// The estimated acceleration of the ball at the last camera frame.
		// 
		point acceleration() const {
			return impl.acceleration() * (flip ? -1.0 : 1.0);
		}

		//
		// The estimated future position of the ball.
		//
		// Parameters:
		//  t
		//   the number of seconds in the future to predict
		//
		point future_position(double t) const {
			return position() + velocity() * t + 0.5 * acceleration() * t * t;
		}

		//
		// Constructs a new ball object.
		//
		// Parameters:
		//  impl
		//   the implementation object that provides global coordinates
		//
		//  flip
		//   whether the X and Y coordinates are reversed for this object
		//
		ball(ball_impl &impl, bool flip) : impl(impl), flip(flip) {
		}

	private:
		ball_impl &impl;
		const bool flip;
};

#endif

