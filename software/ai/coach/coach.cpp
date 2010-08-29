#include "ai/coach/coach.h"
#include <cassert>

using namespace AI;

HL::Strategy::Ptr Coach::get_strategy() const {
	return strategy;
}

const std::vector<HL::StrategyFactory *> &Coach::get_strategies_by_play_type(PlayType::PlayType pt) {
	static bool initialized = false;
	static std::vector<HL::StrategyFactory *> vectors[PlayType::COUNT];

	assert(pt >= 0);
	assert(pt < PlayType::COUNT);

	if (!initialized) {
		for (HL::StrategyFactory::map_type::const_iterator i = HL::StrategyFactory::all().begin(), iend = HL::StrategyFactory::all().end(); i != iend; ++i) {
			HL::StrategyFactory *factory = i->second;
			for (std::size_t j = 0; j < factory->handled_play_types_size; ++j) {
				assert(factory->handled_play_types[j] >= 0);
				assert(factory->handled_play_types[j] < PlayType::COUNT);
				vectors[factory->handled_play_types[j]].push_back(factory);
			}
		}
		initialized = true;
	}

	return vectors[pt];
}

Coach::Coach(World &world) : world(world), strategy() {
}

Coach::~Coach() {
}

void Coach::clear_strategy() {
	set_strategy(HL::Strategy::Ptr());
}

void Coach::set_strategy(const HL::Strategy::Ptr &strat) {
	if (strategy != strat) {
		strategy = strat;
		signal_strategy_changed.emit(strategy);
	}
}

void Coach::set_strategy(const HL::StrategyFactory *fact) {
	set_strategy(fact->create_strategy(world));
}

CoachFactory::CoachFactory(const Glib::ustring &name) : Registerable<CoachFactory>(name) {
}

CoachFactory::~CoachFactory() {
}

