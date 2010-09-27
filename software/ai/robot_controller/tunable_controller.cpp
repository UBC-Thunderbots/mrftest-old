#include "ai/robot_controller/tunable_controller.h"
#include <cassert>
#include <set>

using AI::RC::TunableController;

namespace {
	std::set<TunableController*> instances;
}

TunableController* TunableController::get_instance() {
	if (instances.empty()) return NULL;
	return *instances.begin();
}

TunableController::TunableController() {
	instances.insert(this);
}

TunableController::~TunableController() {
	assert(instances.find(this) != instances.end());
	instances.erase(this);
}

