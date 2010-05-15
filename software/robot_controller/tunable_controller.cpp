#include "robot_controller/tunable_controller.h"

#include <set>
#include <cassert>

namespace {
	std::set<tunable_controller*> instances;
}

tunable_controller* tunable_controller::get_instance() {
	if (instances.empty()) return NULL;
	return *instances.begin();
}

tunable_controller::tunable_controller() {
	instances.insert(this);
}

tunable_controller::~tunable_controller() {
	assert(instances.find(this) != instances.end());
	instances.erase(this);
}

