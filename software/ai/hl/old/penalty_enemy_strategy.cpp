#include "ai/flags.h"
#include "ai/hl/util.h"
#include "ai/hl/old/defender.h"
#include "ai/hl/old/offender.h"
#include "ai/hl/old/strategy.h"
#include "ai/hl/old/tactics.h"
#include "util/dprint.h"
#include "util/param.h"
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;
using namespace AI::HL::Tactics;

namespace {
	const double PENALTY_MARK_LENGTH = 0.45;
	const double RESTRICTED_ZONE_LENGTH = 0.85;

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

			void execute();

			Patrol patrol;
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
		execute();
	}

	void PenaltyEnemyStrategy::execute_penalty_enemy() {
		execute();
	}

	/**
	 * Ticks the strategy
	 */
	void PenaltyEnemyStrategy::execute() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		const Field &f = (world.field());

		Point waypoints[5];

		waypoints[0] = f.friendly_goal();
		waypoints[1] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);
		waypoints[2] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 2 * Robot::MAX_RADIUS);
		waypoints[3] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -2 * Robot::MAX_RADIUS);
		waypoints[4] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		for (size_t i = 1; i < players.size(); ++i) {
			// move the robots to position
			players[i]->move(waypoints[i], (world.ball().position() - players[i]->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
		}

		// let goalie patrol the goal
		const Point starting_position(-0.5 * f.length(), -0.8 * Robot::MAX_RADIUS);
		const Point ending_position(-0.5 * f.length(), 0.8 * Robot::MAX_RADIUS);

		patrol.set_player(players[0]);
		patrol.set_flags(0);
		patrol.set_targets(starting_position, ending_position);
		patrol.tick();
	}

	Strategy::Ptr PenaltyEnemyStrategy::create(World &world) {
		const Strategy::Ptr p(new PenaltyEnemyStrategy(world));
		return p;
	}

	PenaltyEnemyStrategy::PenaltyEnemyStrategy(World &world) : Strategy(world), patrol(world) {
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

