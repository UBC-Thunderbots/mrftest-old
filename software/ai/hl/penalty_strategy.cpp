#include "ai/hl/penalty_friendly.h"
#include "ai/hl/strategy.h"
#include "ai/hl/util.h"

#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

using AI::HL::PenaltyFriendly;

namespace {
	/**
	 * Manages the robots during a penalty.
	 */
	class PenaltyStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * A function to do the assignment of player ("roles").
			 */
			void penalty();

			/**
			 * Creates a new PenaltyStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			PenaltyStrategy(World &world);
			~PenaltyStrategy();
			void on_play_type_changed();
	};

	/**
	 * A factory for constructing \ref PenaltyStrategy "PenaltyStrategies".
	 */
	class PenaltyStrategyFactory : public StrategyFactory {
		public:
			PenaltyStrategyFactory();
			~PenaltyStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of PenaltyStrategyFactory.
	 */
	PenaltyStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::PREPARE_PENALTY_FRIENDLY,

		PlayType::EXECUTE_PENALTY_FRIENDLY,

		PlayType::PREPARE_PENALTY_ENEMY,

		PlayType::EXECUTE_PENALTY_ENEMY
	};

	StrategyFactory &PenaltyStrategy::factory() const {
		return factory_instance;
	}

	void PenaltyStrategy::penalty() {
#warning TODO something sensible
		// std::vector<W::Player::Ptr> players;
		switch (world.playtype()) {
			case PlayType::PREPARE_PENALTY_FRIENDLY:

				break;

			case PlayType::EXECUTE_PENALTY_FRIENDLY:

				break;

			case PlayType::PREPARE_PENALTY_ENEMY:

				break;

			case PlayType::EXECUTE_PENALTY_ENEMY:

				break;

			default:
				break;
		}
	}

	Strategy::Ptr PenaltyStrategy::create(World &world) {
		const Strategy::Ptr p(new PenaltyStrategy(world));
		return p;
	}

	PenaltyStrategy::PenaltyStrategy(World &world) : Strategy(world) {
		world.playtype().signal_changed().connect(sigc::mem_fun(this, &PenaltyStrategy::on_play_type_changed));
	}

	PenaltyStrategy::~PenaltyStrategy() {
	}

	void PenaltyStrategy::on_play_type_changed() {
		for (size_t i = 0; i < G_N_ELEMENTS(HANDLED_PLAY_TYPES); ++i) {
			if (world.playtype() == HANDLED_PLAY_TYPES[i]) {
				return;
			}
		}
		resign();
	}

	PenaltyStrategyFactory::PenaltyStrategyFactory() : StrategyFactory("Penalty", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	PenaltyStrategyFactory::~PenaltyStrategyFactory() {
	}

	Strategy::Ptr PenaltyStrategyFactory::create_strategy(World &world) const {
		return PenaltyStrategy::create(world);
	}
}

