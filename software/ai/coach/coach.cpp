#include "ai/coach/coach.h"
#include <cassert>

using AI::Coach::Coach;
using AI::Coach::CoachFactory;
using namespace AI::Coach::W;

const std::vector<AI::HL::StrategyFactory *> &Coach::get_strategies_by_play_type(AI::Coach::W::PlayType::PlayType pt) {
	static bool initialized = false;
	static std::vector<HL::StrategyFactory *> vectors[PlayType::COUNT];

	assert(pt >= 0);
	assert(pt < PlayType::COUNT);

	if (!initialized) {
		typedef HL::StrategyFactory::Map Map;
		const Map &m = HL::StrategyFactory::all();
		for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
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

Coach::Coach(World &world) : world(world) {
}

Coach::~Coach() {
}

Gtk::Widget *Coach::ui_controls() {
	return 0;
}

CoachFactory::CoachFactory(const Glib::ustring &name) : Registerable<CoachFactory>(name) {
}

CoachFactory::~CoachFactory() {
}

