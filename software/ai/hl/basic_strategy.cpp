#include "ai/hl/basic_strategy.h"
#include "ai/hl/defender.h"
#include "ai/hl/util.h"

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using AI::HL::BasicStrategy;
using namespace AI::HL::W;

namespace {
	/**
	 * A factory for constructing \ref BasicStrategy "BasicStrategies".
	 */
	class BasicStrategyFactory : public StrategyFactory {
		public:
			BasicStrategyFactory();
			~BasicStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of BasicStrategyFactory.
	 */
	BasicStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::PLAY,
	};

	BasicStrategyFactory::BasicStrategyFactory() : StrategyFactory("Basic", HANDLED_PLAY_TYPES, sizeof(HANDLED_PLAY_TYPES) / sizeof(*HANDLED_PLAY_TYPES)) {
	}

	BasicStrategyFactory::~BasicStrategyFactory() {
	}

	Strategy::Ptr BasicStrategyFactory::create_strategy(World &world) const {
		return BasicStrategy::create(world);
	}
}

StrategyFactory &BasicStrategy::factory() const {
	return factory_instance;
}

Strategy::Ptr BasicStrategy::create(World &world) {
	const Strategy::Ptr p(new BasicStrategy(world));
	return p;
}

void BasicStrategy::play() {
#warning work in progress

	// it is easier to change players every tick?
	std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

	// defenders
	Player::Ptr goalie = players[0];
	std::vector<Player::Ptr> defenders;
	for (std::size_t i = 1; i < players.size(); ++i) {
		defenders.push_back(players[i]);
	}

	defender.set_players(defenders, goalie);
	defender.tick();
}

BasicStrategy::BasicStrategy(World &world) : Strategy(world), defender(world) {
	world.playtype().signal_changed().connect(sigc::mem_fun(this, &BasicStrategy::on_play_type_changed));
}

BasicStrategy::~BasicStrategy() {
}

void BasicStrategy::on_play_type_changed() {
	if (world.playtype() != PlayType::PLAY) {
		resign();
	}
}

