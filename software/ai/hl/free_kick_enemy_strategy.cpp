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

namespace {
	
	/**
	 * Manages the robots during a stoppage in place (that is, when the game is in PlayType::STOP).
	 */
	class FreeKickEnemyStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * Creates a new FreeKickEnemyStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			FreeKickEnemyStrategy(World &world);
			~FreeKickEnemyStrategy();

			void execute_indirect_free_kick_enemy();
			void execute_direct_free_kick_enemy();

			void prepare();

			AI::HL::Defender defender;
			AI::HL::Offender offender;

	};

	/**
	 * A factory for constructing \ref FreeKickEnemyStrategy "FreeKickEnemyStrategies".
	 */
	class FreeKickEnemyStrategyFactory : public StrategyFactory {
		public:
			FreeKickEnemyStrategyFactory();
			~FreeKickEnemyStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of FreeKickEnemyStrategyFactory.
	 */
	FreeKickEnemyStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY,
		PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY,
	};

	StrategyFactory &FreeKickEnemyStrategy::factory() const {
		return factory_instance;
	}

	void FreeKickEnemyStrategy::execute_direct_free_kick_enemy() {
		prepare();
	}


// a goal may not be scored directly from the kick
	void FreeKickEnemyStrategy::execute_indirect_free_kick_enemy() {
		prepare();
	}


	void FreeKickEnemyStrategy::prepare() {

		// should have one kicker and the rest as defenders or offenders for friendly freekick

		if (world.friendly_team().size() == 0) {
			return;
		}

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		if (players.size() == 0) {
			return;
		}

		// run assignment and tick
		if (world.playtype() == PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) {

			


		} else if (world.playtype() == PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY) {
			Player::Ptr kicker = players[players.size()-1];
			players.pop_back();

			// 500 mm from the ball
		} else if (world.playtype() == PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY || world.playtype() == PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) {
			std::size_t preferred_offender_number = std::max(1, static_cast<int>(world.friendly_team().size()) - 3);

			// decrease the number of offenders by 1, if there are no offenders, set it to one
			preferred_offender_number--;
			if (preferred_offender_number < 1) {
				preferred_offender_number = 1;
			}

			// preferred_defender_number includes goalie
			std::size_t preferred_defender_number = world.friendly_team().size() - preferred_offender_number;

			std::vector<Player::Ptr> defenders; // excludes goalie
			std::vector<Player::Ptr> offenders;
			// start from 1, to exclude goalie
			for (std::size_t i = 1; i < players.size(); ++i) {
				if (i < preferred_defender_number) {
					defenders.push_back(players[i]);
				} else {
					offenders.push_back(players[i]);
				}
			}

		} else {
			LOG_ERROR("freeKick: unhandled playtype");
		}
	}

	Strategy::Ptr FreeKickEnemyStrategy::create(World &world) {
		const Strategy::Ptr p(new FreeKickEnemyStrategy(world));
		return p;
	}

	FreeKickEnemyStrategy::FreeKickEnemyStrategy(World &world) : Strategy(world), defender(world), offender(world) {}

	FreeKickEnemyStrategy::~FreeKickEnemyStrategy() {
	}

	FreeKickEnemyStrategyFactory::FreeKickEnemyStrategyFactory() : StrategyFactory("FreeKickEnemy", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	FreeKickEnemyStrategyFactory::~FreeKickEnemyStrategyFactory() {
	}

	Strategy::Ptr FreeKickEnemyStrategyFactory::create_strategy(World &world) const {
		return FreeKickEnemyStrategy::create(world);
	}
}

