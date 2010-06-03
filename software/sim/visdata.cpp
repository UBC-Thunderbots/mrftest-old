#include "sim/field.h"
#include "sim/simulator.h"
#include "sim/visdata.h"

simulator_visdata::simulator_visdata(const simulator &sim) : sim(sim) {
}

void simulator_visdata::init() {
	for (std::unordered_map<uint64_t, ::robot::ptr>::const_iterator i = sim.robots().begin(), iend = sim.robots().end(); i != iend; ++i) {
		robots.push_back(i->second);
	}
}

const class visualizable::field &simulator_visdata::field() const {
	return fld;
}

visualizable::ball::ptr simulator_visdata::ball() const {
	return sim.ball();
}

std::size_t simulator_visdata::size() const {
	return robots.size();
}

visualizable::robot::ptr simulator_visdata::operator[](unsigned int index) const {
	return robots[index];
}

bool simulator_visdata::sim_field::valid() const {
	return true;
}

double simulator_visdata::sim_field::length() const {
	return simulator_field::length;
}

double simulator_visdata::sim_field::total_length() const {
	return simulator_field::total_length;
}

double simulator_visdata::sim_field::width() const {
	return simulator_field::width;
}

double simulator_visdata::sim_field::total_width() const {
	return simulator_field::total_width;
}

double simulator_visdata::sim_field::goal_width() const {
	return simulator_field::goal_width;
}

double simulator_visdata::sim_field::centre_circle_radius() const {
	return simulator_field::centre_circle_radius;
}

double simulator_visdata::sim_field::defense_area_radius() const {
	return simulator_field::defense_area_radius;
}

double simulator_visdata::sim_field::defense_area_stretch() const {
	return simulator_field::defense_area_stretch;
}

