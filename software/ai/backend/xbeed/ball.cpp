#include "ai/backend/xbeed/ball.h"

using namespace AI::BE::XBeeD;

Ball::Ball(AI::BE::Backend &backend) : backend(backend), xpred(false), ypred(false) {
	backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &Ball::on_defending_end_changed));
}

Ball::~Ball() {
}

void Ball::update(const Point &pos, const timespec &ts) {
	bool neg = backend.defending_end() == AI::BE::Backend::EAST;
	xpred.add_datum(neg ? -pos.x : pos.x, ts);
	ypred.add_datum(neg ? -pos.y : pos.y, ts);
}

void Ball::lock_time(const timespec &now) {
	xpred.lock_time(now);
	ypred.lock_time(now);
}

Point Ball::position(double delta) const {
	return Point(xpred.value(delta), ypred.value(delta));
}

Point Ball::position(const timespec &ts) const {
	return Point(xpred.value(ts), ypred.value(ts));
}

Point Ball::velocity(double delta) const {
	return Point(xpred.value(delta, 1), ypred.value(delta, 1));
}

Point Ball::velocity(const timespec &ts) const {
	return Point(xpred.value(ts, 1), ypred.value(ts, 1));
}

Point Ball::acceleration(double delta) const {
	return Point(xpred.value(delta, 2), ypred.value(delta, 2));
}

Point Ball::acceleration(const timespec &ts) const {
	return Point(xpred.value(ts, 2), ypred.value(ts, 2));
}

void Ball::on_defending_end_changed() {
	xpred.clear();
	ypred.clear();
}

