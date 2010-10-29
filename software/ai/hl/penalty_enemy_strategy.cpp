#include "ai/flags.h"
#include "ai/hl/defender.h"
#include "ai/hl/offender.h"
#include "ai/hl/strategy.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;
using namespace AI::HL::Tactics;

namespace {
	
	/**
	 * Manages the robots during direct and indirect free kicks.
	 */
	class PenaltyEnemyStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * Creates a new PenaltyEnemyStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			PenaltyEnemyStrategy(World &world);
			~PenaltyEnemyStrategy();

			/**
			 * What this strategy is created for.
			 */
			void prepare_penalty_enemy();
			void execute_penalty_enemy();

			void prepare();

			const static double PENALTY_MARK_LENGTH = 0.45;
			const static double RESTRICTED_ZONE_LENGTH = 0.85;
			const static unsigned int NUMBER_OF_READY_POSITIONS = 4;

			Point ready_positions[NUMBER_OF_READY_POSITIONS];
	};

	/**
	 * A factory for constructing \ref PenaltyEnemyStrategy "PenaltyEnemyStrategies".
	 */
	class PenaltyEnemyStrategyFactory : public StrategyFactory {
		public:
			PenaltyEnemyStrategyFactory();
			~PenaltyEnemyStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of PenaltyEnemyStrategyFactory.
	 */
	PenaltyEnemyStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::PREPARE_PENALTY_ENEMY,
		PlayType::EXECUTE_PENALTY_ENEMY,
	};

	StrategyFactory &PenaltyEnemyStrategy::factory() const {
		return factory_instance;
	}

	void PenaltyEnemyStrategy::prepare_penalty_enemy() {
		prepare();
	}

	void PenaltyEnemyStrategy::execute_penalty_enemy() {
		prepare();
	}

	/**
	 * Ticks the strategy
	 */
	void PenaltyEnemyStrategy::prepare() {
		
		const Field &f = (world.field());
		const Point starting_position(-0.5 * f.length(), -0.5 * Robot::MAX_RADIUS);
		const Point ending_position(-0.5 * f.length(), 0.5 * Robot::MAX_RADIUS);

		ready_positions[0] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);
		ready_positions[1] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 2 * Robot::MAX_RADIUS);
		ready_positions[2] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -2 * Robot::MAX_RADIUS);
		ready_positions[3] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);

		if (world.friendly_team().size() == 0) {
			return;
		}

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		if (players.size() == 0) {
			LOG_WARN("no robots");
			return;
		}

		if (world.playtype() == PlayType::PREPARE_PENALTY_ENEMY) {

			for (size_t i = 1; i < players.size(); ++i) {
				// move the robots to position
				players[i]->move(ready_positions[i-1], (ready_positions[i-1] - players[i]->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}

		} else if (world.playtype() == PlayType::EXECUTE_PENALTY_ENEMY) {

			// let goalie patrol the goal
			Patrol(world, players[0], starting_position, ending_position, AI::Flags::calc_flags(world.playtype())).tick();
			
		} else {
			LOG_WARN("penalty_enemy: unhandled playtype");
			return;
		}
		
	}

	Strategy::Ptr PenaltyEnemyStrategy::create(World &world) {
		const Strategy::Ptr p(new PenaltyEnemyStrategy(world));
		return p;
	}

	PenaltyEnemyStrategy::PenaltyEnemyStrategy(World &world) : Strategy(world) {
	}

	PenaltyEnemyStrategy::~PenaltyEnemyStrategy() {
	}

	PenaltyEnemyStrategyFactory::PenaltyEnemyStrategyFactory() : StrategyFactory("PenaltyEnemy", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	PenaltyEnemyStrategyFactory::~PenaltyEnemyStrategyFactory() {
	}

	Strategy::Ptr PenaltyEnemyStrategyFactory::create_strategy(World &world) const {
		return PenaltyEnemyStrategy::create(world);
	}
}

