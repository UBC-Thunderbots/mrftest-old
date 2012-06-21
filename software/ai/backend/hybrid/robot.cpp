#include "ai/backend/hybrid/robot.h"
#include "ai/backend/hybrid/hybrid_backend.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace AI::BE::Hybrid;

namespace {
	DoubleParam linear_decay_constant("Robot Linear Decay Constant", "Backend/Hybrid", 99.0, 0.0, 100.0);
	DoubleParam angular_decay_constant("Robot Angular Decay Constant", "Backend/Hybrid", 99.0, 0.0, 100.0);
}

Robot::Robot(AI::BE::Backend &backend, unsigned int pattern) : seen_this_frame(false), vision_failures(0), backend(backend), pattern_(pattern), pred(1.3e-3, 2, linear_decay_constant, Angle::of_radians(1.3e-3), Angle::of_radians(2), angular_decay_constant) {
	backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &Robot::on_defending_end_changed));
}

void Robot::update(const SSL_DetectionRobot &packet, timespec ts) {
	if (packet.has_orientation()) {
		bool neg = backend.defending_end() == AI::BE::Backend::FieldEnd::EAST;
		Point pos(neg ? -packet.x() / 1000.0 : packet.x() / 1000.0, neg ? -packet.y() / 1000.0 : packet.y() / 1000.0);
		Angle ori = (Angle::of_radians(packet.orientation()) + (neg ? Angle::HALF : Angle::ZERO)).angle_mod();
		pred.add_measurement(pos, ori, timespec_sub(ts, double_to_timespec(HYBRID_LOOP_DELAY)));
	} else {
		LOG_WARN("Vision packet has robot with no orientation.");
	}
}

void Robot::lock_time(timespec now) {
	pred.lock_time(now);
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
	return pred.value(delta).first.first;
}

Point Robot::velocity(double delta) const {
	return pred.value(delta, 1).first.first;
}

Point Robot::position_stdev(double delta) const {
	return pred.value(delta).second.first;
}

Point Robot::velocity_stdev(double delta) const {
	return pred.value(delta, 1).second.first;
}

Angle Robot::orientation(double delta) const {
	return pred.value(delta).first.second;
}

Angle Robot::avelocity(double delta) const {
	return pred.value(delta, 1).first.second;
}

Angle Robot::orientation_stdev(double delta) const {
	return pred.value(delta).second.second;
}

Angle Robot::avelocity_stdev(double delta) const {
	return pred.value(delta, 1).second.second;
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

void Robot::on_defending_end_changed() {
	pred.clear();
}

