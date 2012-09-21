#include "ai/backend/ball.h"
#include "ai/backend/backend.h"
#include "util/param.h"

using AI::BE::Ball;

namespace {
	DoubleParam BALL_DECAY_CONSTANT("Ball Decay Constant", "Backend", 99.0, 0.0, 100.0);
}

Ball::Ball() : should_highlight(false), pred(1.3e-3, 2, BALL_DECAY_CONSTANT) {
}

void Ball::add_field_data(Point pos, timespec ts) {
	pred.add_measurement(pos, timespec_sub(ts, double_to_timespec(LOOP_DELAY)));
}

void Ball::lock_time(timespec now) {
	pred.lock_time(now);
}

Point Ball::position(double delta) const {
	return pred.value(delta).first;
}

Point Ball::velocity(double delta) const {
	return pred.value(delta, 1).first;
}

Point Ball::position_stdev(double delta) const {
	return pred.value(delta).second;
}

Point Ball::velocity_stdev(double delta) const {
	return pred.value(delta, 1).second;
}

bool Ball::highlight() const {
	return should_highlight;
}

Visualizable::Colour Ball::highlight_colour() const {
	return Visualizable::Colour(0.0, 0.0, 0.0);
}

