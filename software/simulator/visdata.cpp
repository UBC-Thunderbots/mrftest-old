#include "simulator/field.h"
#include "simulator/simulator.h"
#include "simulator/visdata.h"

SimulatorVisData::SimulatorVisData(const Simulator &sim) : sim(sim) {
}

void SimulatorVisData::init() {
	for (std::unordered_map<uint64_t, RefPtr<SimulatorRobot> >::const_iterator i = sim.robots().begin(), iend = sim.robots().end(); i != iend; ++i) {
		robots.push_back(i->second);
	}
}

const class Visualizable::Field &SimulatorVisData::field() const {
	return fld;
}

RefPtr<Visualizable::Ball> SimulatorVisData::ball() const {
	return sim.ball();
}

std::size_t SimulatorVisData::size() const {
	return robots.size();
}

RefPtr<Visualizable::Robot> SimulatorVisData::operator[](unsigned int index) const {
	return robots[index];
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

