#ifndef WORLD_BALL_H
#define WORLD_BALL_H

#include <cstddef>
#include <glibmm.h>
#include "geom/point.h"
#include "util/circular_buffer.h"
#include "util/byref.h"

//
// The ball.
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
		const point &position() const;

		//
		// The estimated velocity of the ball at the last camera frame.
		//
		const point &velocity() const;

		//
		// The estimated acceleration of the ball at the last camera frame.
		// 
		const point &acceleration() const;

		//
		// The estimated future position of the ball.
		//
		// Parameters:
		//  t
		//   the number of seconds in the future to predict
		//
		point future_position(double t) const;

	protected:
		//
		// Sets the current position of the ball.
		//
		void set_position(const point &pt, double timestamp);

	private:
		//
		// How many past positions to keep for estimation purposes.
		//
		static const std::size_t NUM_PAST_POSITIONS = 6;

		//
		// The list of positions.
		//
		circular_buffer<point, NUM_PAST_POSITIONS> past_positions;

		//
		// The list of timestamps.
		//
		circular_buffer<double, NUM_PAST_POSITIONS> past_timestamps;
};

#endif

