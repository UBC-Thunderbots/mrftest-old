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
	 * Manages the robots during direct and indirect free kicks.
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
			void on_player_added(std::size_t);
			void on_player_removing(std::size_t);

			void execute_indirect_free_kick_enemy();
			void execute_direct_free_kick_enemy();

			void prepare();

			void run_assignment();

			// the players invovled
			std::vector<Player::Ptr> defenders;
			std::vector<Player::Ptr> offenders;

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

	void FreeKickEnemyStrategy::execute_indirect_free_kick_enemy() {
		prepare();
	}

	/**
	 * Ticks the strategy
	 *
	 * Don't need to assign a kicker as in friendly. It is also "ok" for players to ram the enemy as long as they block the ball.
	 */
	void FreeKickEnemyStrategy::prepare() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		if (players.size() == 0) {
			return;
		}

		if (world.playtype() == PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY || world.playtype() == PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) {
			defender.tick();
			offender.tick();
		} else {
			LOG_ERROR("freeKickEnemy: unhandled playtype");
		}
	}

	void FreeKickEnemyStrategy::run_assignment() {
		if (world.friendly_team().size() == 0) {
			LOG_WARN("no players");
			return;
		}

		// it is easier to change players every tick?
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		// sort players by distance to own goal
		if (players.size() > 1) {
			std::sort(players.begin() + 1, players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.field().friendly_goal()));
		}

		// defenders
		Player::Ptr goalie = players[0];

		std::size_t ndefenders = 1; // includes goalie

		switch (players.size()) {
			case 5:
			case 4:
				++ndefenders;

			case 3:
			case 2:
				break;
		}

		// clear up
		defenders.clear();
		offenders.clear();

		// start from 1, to exclude goalie
		for (std::size_t i = 1; i < players.size(); ++i) {
			if (i < ndefenders) {
				defenders.push_back(players[i]);
			} else {
				offenders.push_back(players[i]);
			}
		}

		LOG_INFO(Glib::ustring::compose("player reassignment %1 defenders, %2 offenders", ndefenders, offenders.size()));

		defender.set_players(defenders, goalie);
		offender.set_players(offenders);
	}

	Strategy::Ptr FreeKickEnemyStrategy::create(World &world) {
		const Strategy::Ptr p(new FreeKickEnemyStrategy(world));
		return p;
	}

	FreeKickEnemyStrategy::FreeKickEnemyStrategy(World &world) : Strategy(world), defender(world), offender(world) {
		world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &FreeKickEnemyStrategy::on_player_added));
		world.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &FreeKickEnemyStrategy::on_player_removing));
		run_assignment();
	}

	void FreeKickEnemyStrategy::on_player_added(std::size_t) {
		run_assignment();
	}

	void FreeKickEnemyStrategy::on_player_removing(std::size_t) {
		run_assignment();
	}

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

