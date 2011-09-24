#include "ai/backend/xbee/ball.h"
#include "ai/backend/xbee/xbee_backend.h"

using namespace AI::BE::XBee;

namespace {
	DoubleParam BALL_DECAY_CONSTANT("Ball Decay Constant", "Backend/XBee", 99.0, 0.0, 100.0);
}

Ball::Ball(AI::BE::Backend &backend) : backend(backend), pred(1.3e-3, 2, BALL_DECAY_CONSTANT) {
	backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &Ball::on_defending_end_changed));
}

void Ball::update(Point pos, timespec ts) {
	bool neg = backend.defending_end() == AI::BE::Backend::FieldEnd::EAST;
	pred.add_measurement(neg ? -pos : pos, timespec_sub(ts, double_to_timespec(LOOP_DELAY)));
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
	return false;
}

Visualizable::Colour Ball::highlight_colour() const {
	return Visualizable::Colour(0.0, 0.0, 0.0);
}

void Ball::on_defending_end_changed() {
	pred.clear();
}

