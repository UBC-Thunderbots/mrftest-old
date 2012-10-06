#include "ai/backend/robot.h"
#include "ai/backend/backend.h"
#include <stdexcept>

using AI::BE::Robot;

Robot::Robot(unsigned int pattern) : pred(1.3e-3, 2, 99.0, Angle::of_radians(1.3e-3), Angle::of_radians(2), 99.0), pattern_(pattern), avoid_distance_(AI::Flags::AvoidDistance::MEDIUM) {
}

void Robot::add_field_data(Point pos, Angle ori, timespec ts) {
	pred.add_measurement(pos, ori, timespec_sub(ts, double_to_timespec(AI::BE::LOOP_DELAY)));
}

ObjectStore &Robot::object_store() const {
	return object_store_;
}

unsigned int Robot::pattern() const {
	return pattern_;
}

Point Robot::position(double delta) const {
	return pred.value(delta).first.first;
}

Point Robot::position_stdev(double delta) const {
	return pred.value(delta).second.first;
}

Angle Robot::orientation(double delta) const {
	return pred.value(delta).first.second;
}

Angle Robot::orientation_stdev(double delta) const {
	return pred.value(delta).second.second;
}

Point Robot::velocity(double delta) const {
	return pred.value(delta, 1).first.first;
}

Point Robot::velocity_stdev(double delta) const {
	return pred.value(delta, 1).second.first;
}

Angle Robot::avelocity(double delta) const {
	return pred.value(delta, 1).first.second;
}

Angle Robot::avelocity_stdev(double delta) const {
	return pred.value(delta, 1).second.second;
}

bool Robot::has_destination() const {
	return false;
}

std::pair<Point, Angle> Robot::destination() const {
	throw std::logic_error("This robot has no destination");
}

bool Robot::has_path() const {
	return false;
}

const std::vector<std::pair<std::pair<Point, Angle>, timespec> > &Robot::path() const {
	throw std::logic_error("This robot has no path");
}

void Robot::pre_tick() {
	avoid_distance_ = AI::Flags::AvoidDistance::MEDIUM;
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

unsigned int Robot::num_bar_graphs() const {
	return 0;
}

double Robot::bar_graph_value(unsigned int) const {
	throw std::logic_error("This robot has no graphs");
}

Visualizable::Colour Robot::bar_graph_colour(unsigned int) const {
	throw std::logic_error("This robot has no graphs");
}
