#ifndef SIMULATOR_BALL_H
#define SIMULATOR_BALL_H

#include "world/ball_impl.h"

//
// An extension of ball_impl that defines functions that are only needed for
// simulation implementations.
//
class simulator_ball_impl : public ball_impl {
	public:
		//
		// A pointer to a simulator_ball_impl.
		//
		typedef Glib::RefPtr<simulator_ball_impl> ptr;

		//
		// Checks whether the ball is in a goal. This should take into account
		// engine-specific data regarding the ball, such as altitude. The
		// determination of WHICH goal the ball is in is made by the sign of its
		// X coordinate.
		//
		virtual bool in_goal() = 0;

		//
		// A trivial implementation.
		//
		static const ptr &trivial();
};

#endif

