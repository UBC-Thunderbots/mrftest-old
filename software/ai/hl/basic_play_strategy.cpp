#include "ai/hl/strategy.h"
#include "ai/hl/defender.h"
#include "ai/hl/offender.h"
#include "ai/hl/util.h"

using AI::HL::Defender;
using AI::HL::Offender;
using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {
	/**
	 * A full implementation of a strategy that handles normal play.
	 */
	class BasicPlayStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * The reason we have this class.
			 */
			void play();

			static Strategy::Ptr create(AI::HL::W::World &world);

		private:
			BasicPlayStrategy(AI::HL::W::World &world);
			~BasicPlayStrategy();
			void on_play_type_changed();

			Defender defender;
			Offender offender;
	};

	/**
	 * A factory for constructing \ref BasicPlayStrategy "BasicPlayStrategies".
	 */
	class BasicPlayStrategyFactory : public StrategyFactory {
		public:
			BasicPlayStrategyFactory();
			~BasicPlayStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of BasicPlayStrategyFactory.
	 */
	BasicPlayStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::PLAY,
	};

	BasicPlayStrategyFactory::BasicPlayStrategyFactory() : StrategyFactory("BasicPlay", HANDLED_PLAY_TYPES, sizeof(HANDLED_PLAY_TYPES) / sizeof(*HANDLED_PLAY_TYPES)) {
	}

	BasicPlayStrategyFactory::~BasicPlayStrategyFactory() {
	}

	Strategy::Ptr BasicPlayStrategyFactory::create_strategy(World &world) const {
		return BasicPlayStrategy::create(world);
	}

	StrategyFactory &BasicPlayStrategy::factory() const {
		return factory_instance;
	}

	Strategy::Ptr BasicPlayStrategy::create(World &world) {
		const Strategy::Ptr p(new BasicPlayStrategy(world));
		return p;
	}

	void BasicPlayStrategy::play() {
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

		if (offenders.size() > 0) {
			offender.set_players(offenders);
			offender.set_chase(offender_chase);
			offender.tick();
		}

		defender.set_players(defenders, goalie);
		defender.set_chase(!offender_chase);
		defender.tick();
	}

	BasicPlayStrategy::BasicPlayStrategy(World &world) : Strategy(world), defender(world), offender(world) {
		world.playtype().signal_changed().connect(sigc::mem_fun(this, &BasicPlayStrategy::on_play_type_changed));
	}

	BasicPlayStrategy::~BasicPlayStrategy() {
	}

	void BasicPlayStrategy::on_play_type_changed() {
		if (world.playtype() != PlayType::PLAY) {
			resign();
		}
	}

}

