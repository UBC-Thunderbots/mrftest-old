#include "ai/backend/xbee/ball.h"
#include "ai/backend/xbee/xbee_backend.h"

using namespace AI::BE::XBee;

Ball::Ball(AI::BE::Backend &backend) : backend(backend), xpred(false, 1.3e-3, 2), ypred(false, 1.3e-3, 2) {
	backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &Ball::on_defending_end_changed));
}

void Ball::update(const Point &pos, const timespec &ts) {
	bool neg = backend.defending_end() == AI::BE::Backend::FieldEnd::EAST;
	xpred.add_datum(neg ? -pos.x : pos.x, timespec_sub(ts,double_to_timespec(LOOP_DELAY)));
	ypred.add_datum(neg ? -pos.y : pos.y, timespec_sub(ts,double_to_timespec(LOOP_DELAY)));
}

void Ball::lock_time(const timespec &now) {
	xpred.lock_time(now);
	ypred.lock_time(now);
}

Point Ball::position(double delta) const {
	return Point(xpred.value(delta).first, ypred.value(delta).first);
}

Point Ball::velocity(double delta) const {
	return Point(xpred.value(delta, 1).first, ypred.value(delta, 1).first);
}

bool Ball::highlight() const {
	return false;
}

Visualizable::Colour Ball::highlight_colour() const {
	return Visualizable::Colour(0.0, 0.0, 0.0);
}

void Ball::on_defending_end_changed() {
	xpred.clear();
	ypred.clear();
}

