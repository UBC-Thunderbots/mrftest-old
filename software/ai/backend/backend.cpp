#include "ai/backend/backend.h"

using AI::BE::Backend;
using AI::BE::BackendFactory;

void Backend::strategy(AI::HL::StrategyFactory *s) {
	if (s) {
		strategy_ = s->create_strategy(*this);
	} else {
		strategy_ = AI::HL::Strategy::Ptr();
	}
}

Backend::Backend() : defending_end_(WEST), friendly_colour_(YELLOW), playtype_(AI::Common::PlayType::HALT), playtype_override_(AI::Common::PlayType::COUNT), ball_filter_(0), strategy_(AI::HL::Strategy::Ptr()) {
}

Backend::~Backend() {
}

BackendFactory::BackendFactory(const char *name) : Registerable<BackendFactory>(name) {
}

BackendFactory::~BackendFactory() {
}

