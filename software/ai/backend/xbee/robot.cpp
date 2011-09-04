#include "ai/backend/xbee/robot.h"
#include "ai/backend/xbee/xbee_backend.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace AI::BE::XBee;

namespace {
	DoubleParam LINEAR_DECAY_CONSTANT("Robot Linear Decay Constant","Backend/XBee",0.0,99.0,100.0);
	DoubleParam ANGULAR_DECAY_CONSTANT("Robot Angular Decay Constant","Backend/XBee",0.0,99.0,100.0);
}

Robot::Ptr Robot::create(AI::BE::Backend &backend, unsigned int pattern) {
	Ptr p(new Robot(backend, pattern));
	return p;
}

void Robot::update(const SSL_DetectionRobot &packet, const timespec &ts) {
	if (packet.has_orientation()) {
		bool neg = backend.defending_end() == AI::BE::Backend::FieldEnd::EAST;
		xpred.add_datum(neg ? -packet.x() / 1000.0 : packet.x() / 1000.0, timespec_sub(ts, double_to_timespec(LOOP_DELAY)));
		ypred.add_datum(neg ? -packet.y() / 1000.0 : packet.y() / 1000.0, timespec_sub(ts, double_to_timespec(LOOP_DELAY)));
		tpred.add_datum((Angle::of_radians(packet.orientation()) + (neg ? Angle::HALF : Angle::ZERO)).angle_mod(), timespec_sub(ts, double_to_timespec(LOOP_DELAY)));
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
	return Point(xpred.value(delta).first, ypred.value(delta).first);
}

Point Robot::velocity(double delta) const {
	return Point(xpred.value(delta, 1).first, ypred.value(delta, 1).first);
}

Point Robot::position_stdev(double delta) const {
	return Point(xpred.value(delta).second, ypred.value(delta).second);
}

Point Robot::velocity_stdev(double delta) const {
	return Point(xpred.value(delta, 1).second, ypred.value(delta, 1).second);
}

Angle Robot::orientation(double delta) const {
	return tpred.value(delta).first;
}

Angle Robot::avelocity(double delta) const {
	return tpred.value(delta, 1).first;
}

Angle Robot::orientation_stdev(double delta) const {
	return tpred.value(delta).second;
}

Angle Robot::avelocity_stdev(double delta) const {
	return tpred.value(delta, 1).second;
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

const std::pair<Point, Angle> &Robot::destination() const {
	throw std::logic_error("This robot has no destination");
}

bool Robot::has_path() const {
	return false;
}

const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &Robot::path() const {
	throw std::logic_error("This robot has no path");
}

unsigned int Robot::num_bar_graphs() const {
	return 0;
}

double Robot::bar_graph_value(unsigned int) const {
	throw std::logic_error("This robot has no graphs");
}

Visualizable::Colour Robot::bar_graph_colour(unsigned int) const {
	throw std::logic_error("This robot has no graphs");
}

Robot::Robot(AI::BE::Backend &backend, unsigned int pattern) : seen_this_frame(false), vision_failures(0), backend(backend), pattern_(pattern), xpred(1.3e-3, 2, LINEAR_DECAY_CONSTANT), ypred(1.3e-3, 2, LINEAR_DECAY_CONSTANT), tpred(Angle::of_radians(1.3e-3), Angle::of_radians(2), ANGULAR_DECAY_CONSTANT) {
	backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &Robot::on_defending_end_changed));
}

Robot::~Robot() = default;

void Robot::on_defending_end_changed() {
	xpred.clear();
	ypred.clear();
	tpred.clear();
}

