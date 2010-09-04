#include "simulator/field.h"
#include "simulator/simulator.h"
#include "simulator/visdata.h"

SimulatorVisData::SimulatorVisData(const Simulator &sim) : sim(sim), ball_(sim.ball()) {
}

void SimulatorVisData::init() {
	for (std::unordered_map<uint64_t, SimulatorRobot::Ptr>::const_iterator i = sim.robots().begin(), iend = sim.robots().end(); i != iend; ++i) {
		robots.push_back(SimulatorVisRobot(i->second));
	}
}

const class Visualizable::Field &SimulatorVisData::field() const {
	return fld;
}

const Visualizable::Ball &SimulatorVisData::ball() const {
	return ball_;
}

std::size_t SimulatorVisData::size() const {
	return robots.size();
}

const Visualizable::Robot &SimulatorVisData::operator[](unsigned int index) const {
	return robots[index];
}

SimulatorVisData::SimulatorVisField::SimulatorVisField() {
}

SimulatorVisData::SimulatorVisField::~SimulatorVisField() {
}

bool SimulatorVisData::SimulatorVisField::valid() const {
	return true;
}

double SimulatorVisData::SimulatorVisField::length() const {
	return SimulatorField::length;
}

double SimulatorVisData::SimulatorVisField::total_length() const {
	return SimulatorField::total_length;
}

double SimulatorVisData::SimulatorVisField::width() const {
	return SimulatorField::width;
}

double SimulatorVisData::SimulatorVisField::total_width() const {
	return SimulatorField::total_width;
}

double SimulatorVisData::SimulatorVisField::goal_width() const {
	return SimulatorField::goal_width;
}

double SimulatorVisData::SimulatorVisField::centre_circle_radius() const {
	return SimulatorField::centre_circle_radius;
}

double SimulatorVisData::SimulatorVisField::defense_area_radius() const {
	return SimulatorField::defense_area_radius;
}

double SimulatorVisData::SimulatorVisField::defense_area_stretch() const {
	return SimulatorField::defense_area_stretch;
}

SimulatorVisData::SimulatorVisRobot::SimulatorVisRobot(SimulatorRobot::Ptr bot) : bot(bot) {
}

SimulatorVisData::SimulatorVisRobot::~SimulatorVisRobot() {
}

Point SimulatorVisData::SimulatorVisRobot::position() const {
	return bot->position();
}

double SimulatorVisData::SimulatorVisRobot::orientation() const {
	return bot->orientation();
}

bool SimulatorVisData::SimulatorVisRobot::visualizer_visible() const {
	return bot->has_player();
}

Visualizable::RobotColour SimulatorVisData::SimulatorVisRobot::visualizer_colour() const {
	return bot->yellow() ? Visualizable::RobotColour(1.0, 1.0, 0.0) : Visualizable::RobotColour(0.0, 0.0, 1.0);
}

Glib::ustring SimulatorVisData::SimulatorVisRobot::visualizer_label() const {
	return Glib::ustring::compose("%1%2", bot->yellow() ? 'Y' : 'B', bot->pattern_index());
}

bool SimulatorVisData::SimulatorVisRobot::has_destination() const {
	return false;
}

Point SimulatorVisData::SimulatorVisRobot::destination() const {
	std::abort();
}

SimulatorVisData::SimulatorVisBall::SimulatorVisBall(SimulatorBall::Ptr ball) : ball(ball) {
}

SimulatorVisData::SimulatorVisBall::~SimulatorVisBall() {
}

Point SimulatorVisData::SimulatorVisBall::position() const {
	return ball->position();
}

Point SimulatorVisData::SimulatorVisBall::velocity() const {
	return ball->velocity();
}

