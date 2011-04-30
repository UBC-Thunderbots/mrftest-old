#include "ai/backend/xbee/robot.h"
#include "ai/backend/xbee/xbee_backend.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace AI::BE::XBee;

Robot::Ptr Robot::create(AI::BE::Backend &backend, unsigned int pattern) {
	Ptr p(new Robot(backend, pattern));
	return p;
}

void Robot::update(const SSL_DetectionRobot &packet, const timespec &ts) {
	if (packet.has_orientation()) {
		bool neg = backend.defending_end() == AI::BE::Backend::FieldEnd::EAST;
		xpred.add_datum(neg ? -packet.x() / 1000.0 : packet.x() / 1000.0, timespec_sub(ts,double_to_timespec(LOOP_DELAY)));
		ypred.add_datum(neg ? -packet.y() / 1000.0 : packet.y() / 1000.0, timespec_sub(ts,double_to_timespec(LOOP_DELAY)));
		tpred.add_datum(angle_mod(packet.orientation() + (neg ? M_PI : 0.0)), timespec_sub(ts,double_to_timespec(LOOP_DELAY)));
	} else {
		LOG_WARN("Vision packet has robot with no orientation.");
	}
}

void Robot::lock_time(const timespec &now) {
	xpred.lock_time(now);
	ypred.lock_time(now);
	tpred.lock_time(now);
}

Visualizable::Colour Robot::visualizer_colour() const {
	return Visualizable::Colour(1.0, 0.0, 0.0);
}

Glib::ustring Robot::visualizer_label() const {
	return Glib::ustring::format(pattern());
}

bool Robot::highlight() const {
	return false;
}

Visualizable::Colour Robot::highlight_colour() const {
	return Visualizable::Colour(0.0, 0.0, 0.0);
}

Point Robot::position(double delta) const {
	return Point(xpred.value(delta), ypred.value(delta));
}

Point Robot::velocity(double delta) const {
	return Point(xpred.value(delta, 1), ypred.value(delta, 1));
}

double Robot::orientation(double delta) const {
	return tpred.value(delta);
}

double Robot::avelocity(double delta) const {
	return tpred.value(delta, 1);
}

unsigned int Robot::pattern() const {
	return pattern_;
}

ObjectStore &Robot::object_store() const {
	return object_store_;
}

bool Robot::has_destination() const {
	return false;
}

const std::pair<Point, double> &Robot::destination() const {
	throw std::logic_error("This robot has no destination");
}

bool Robot::has_path() const {
	return false;
}

const std::vector<std::pair<std::pair<Point, double>, timespec> > &Robot::path() const {
	throw std::logic_error("This robot has no path");
}

Robot::Robot(AI::BE::Backend &backend, unsigned int pattern) : seen_this_frame(false), vision_failures(0), backend(backend), pattern_(pattern), xpred(false, 1.3e-3, 2), ypred(false, 1.3e-3, 2), tpred(true, 1.3e-3, 2) {
	backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &Robot::on_defending_end_changed));
}

Robot::~Robot() = default;

void Robot::on_defending_end_changed() {
	xpred.clear();
	ypred.clear();
	tpred.clear();
}

