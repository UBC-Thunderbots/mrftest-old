#ifndef WORLD_BALL_H
#define WORLD_BALL_H

#include "world/ball_impl.h"
#include <cassert>

//
// The ball, as seen by the AI. Vectors in this class are in team coordinates.
//
class ball : public draggable {
	public:
		//
		// A pointer to a ball object.
		//
		typedef Glib::RefPtr<ball> ptr;

		//
		// The position of the ball at the last camera frame.
		//
		point position() const __attribute__((warn_unused_result)) {
			return impl->position() * (flip ? -1.0 : 1.0);
		}

		//
		// The estimated velocity of the ball at the last camera frame.
		//
		point est_velocity() const __attribute__((warn_unused_result)) {
			return impl->est_velocity() * (flip ? -1.0 : 1.0);
		}

		//
		// Allows the UI to set the position of the ball.
		//
		void ext_drag(const point &p, const point &v) {
			impl->ext_drag(p * (flip ? -1.0 : 1.0), v * (flip ? -1.0 : 1.0));
		}

		//
		// Sets a new ball_impl to delegate to. This method is intended only for
		// use by universe implementers and should not be called by the AI.
		//
		void set_impl(const ball_impl::ptr &i) {
			assert(i);
			impl = i;
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
		ball(const ball_impl::ptr &impl, bool flip) : impl(impl), flip(flip) {
			assert(impl);
		}

		//
		// The approximate radius of the ball.
		//
		static const double RADIUS = 0.0215;

	private:
		ball_impl::ptr impl;
		const bool flip;
};

#endif

