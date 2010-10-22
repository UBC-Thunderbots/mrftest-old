#include "ai/hl/penalty_enemy.h"
#include "ai/hl/penalty_friendly.h"
#include "ai/hl/strategy.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

using AI::HL::PenaltyFriendly;
using AI::HL::PenaltyEnemy;

namespace {
	/**
	 * Manages the robots during a penalty.
	 */
	class PenaltyStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * What this strategy is created for.
			 */
			void prepare_penalty_friendly();
			void execute_penalty_friendly();

			void prepare_penalty_enemy();
			void execute_penalty_enemy();

			/**
			 * Creates a new PenaltyStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			PenaltyStrategy(World &world);
			~PenaltyStrategy();

			void prepare();

			PenaltyFriendly pFriendly;
			PenaltyEnemy pEnemy;
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
		PlayType::EXECUTE_PENALTY_ENEMY,
	};

	StrategyFactory &PenaltyStrategy::factory() const {
		return factory_instance;
	}

	void PenaltyStrategy::prepare_penalty_friendly() {
		prepare();
	}

	void PenaltyStrategy::execute_penalty_friendly() {
		prepare();
	}

	void PenaltyStrategy::prepare_penalty_enemy() {
		prepare();
	}

	void PenaltyStrategy::execute_penalty_enemy() {
		prepare();
	}

	void PenaltyStrategy::prepare() {
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		if (players.size() == 0) {
			return;
		}
		// run assignment and tick
		if (world.playtype() == PlayType::PREPARE_PENALTY_FRIENDLY || world.playtype() == PlayType::EXECUTE_PENALTY_FRIENDLY) {
			players.pop_back();
			pFriendly.set_players(players);
			pFriendly.tick();
		} else if (world.playtype() == PlayType::PREPARE_PENALTY_ENEMY || world.playtype() == PlayType::EXECUTE_PENALTY_ENEMY) {
			Player::Ptr goalie = players[4];
			players.pop_back();
			pEnemy.set_players(players, goalie);
			pEnemy.tick();
		} else {
			LOG_ERROR("penalty_enemy: unhandled playtype");
		}
	}

	Strategy::Ptr PenaltyStrategy::create(World &world) {
		const Strategy::Ptr p(new PenaltyStrategy(world));
		return p;
	}

	PenaltyStrategy::PenaltyStrategy(World &world) : Strategy(world), pFriendly(world), pEnemy(world) {
	}

	PenaltyStrategy::~PenaltyStrategy() {
	}

	PenaltyStrategyFactory::PenaltyStrategyFactory() : StrategyFactory("Penalty", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	PenaltyStrategyFactory::~PenaltyStrategyFactory() {
	}

	Strategy::Ptr PenaltyStrategyFactory::create_strategy(World &world) const {
		return PenaltyStrategy::create(world);
	}
}

