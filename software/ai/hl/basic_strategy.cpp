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
	if (world.friendly_team().size() == 0) {
		return;
	}

	// it is easier to change players every tick?
	std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

	// defenders
	Player::Ptr goalie = players[0];
	std::vector<Player::Ptr> defenders;
	std::vector<Player::Ptr> offenders;

	switch (players.size()) {
		case 5:
			defenders.push_back(players[4]);

		case 4:
			offenders.push_back(players[3]);

		case 3:
			defenders.push_back(players[2]);

		case 2:
			offenders.push_back(players[1]);
			break;
	}

	// see who has the closest ball
	bool offender_chase = true;
	double best_dist = 1e99;
	for (std::size_t i = 0; i < offenders.size(); ++i) {
		best_dist = std::min(best_dist, (offenders[i]->position() - world.ball().position()).len());
	}
	for (std::size_t i = 0; i < defenders.size(); ++i) {
		double dist = (offenders[i]->position() - world.ball().position()).len();
		if (dist < best_dist) {
			offender_chase = false;
			break;
		}
	}

	offender.set_players(offenders);
	offender.set_chase(offender_chase);
	offender.tick();

	defender.set_players(defenders, goalie);
	defender.set_chase(!offender_chase);
	defender.tick();
}

BasicStrategy::BasicStrategy(World &world) : Strategy(world), defender(world), offender(world) {
	world.playtype().signal_changed().connect(sigc::mem_fun(this, &BasicStrategy::on_play_type_changed));
}

BasicStrategy::~BasicStrategy() {
}

void BasicStrategy::on_play_type_changed() {
	if (world.playtype() != PlayType::PLAY) {
		resign();
	}
}

