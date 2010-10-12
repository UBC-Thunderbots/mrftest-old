#include "ai/flags.h"
#include "ai/hl/defender.h"
#include "ai/hl/strategy.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "uicomponents/param.h"
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {
	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = 0.050 + Robot::MAX_RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const double AVOIDANCE_ANGLE = 2.0 * asin(Robot::MAX_RADIUS / AVOIDANCE_DIST);

	DoubleParam separation_angle("angle to separate players (degrees)", 5, 0, 20);

	/**
	 * Manages the robots during a stoppage in place (that is, when the game is in PlayType::STOP).
	 */
	class KickoffEnemyStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * Creates a new KickoffEnemyStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			KickoffEnemyStrategy(World &world);
			~KickoffEnemyStrategy();

			void on_play_type_changed();

			//void prepare_kickoff_friendly();
			//void execute_kickoff_friendly();

			void prepare_kickoff_enemy();
			void execute_kickoff_enemy();

			void prepare();

			AI::HL::Defender defender;
	};

	/**
	 * A factory for constructing \ref KickoffEnemyStrategy "StopStrategies".
	 */
	class KickoffEnemyStrategyFactory : public StrategyFactory {
		public:
			KickoffEnemyStrategyFactory();
			~KickoffEnemyStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of KickoffEnemyStrategyFactory.
	 */
	KickoffEnemyStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::PREPARE_KICKOFF_ENEMY,
		PlayType::EXECUTE_KICKOFF_ENEMY,
	};

	StrategyFactory &KickoffEnemyStrategy::factory() const {
		return factory_instance;
	}

	//void KickoffEnemyStrategy::prepare_kickoff_friendly() {
		//prepare();
	//}

	//void KickoffEnemyStrategy::execute_kickoff_friendly() {
		// TODO
	//}

	void KickoffEnemyStrategy::prepare_kickoff_enemy() {
		prepare();
	}

	void KickoffEnemyStrategy::execute_kickoff_enemy() {
		prepare();
	}

	void KickoffEnemyStrategy::prepare() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		// it is easier to change players every tick?
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		// sort players by distance to own goal
		std::sort(players.begin() + 1, players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.field().friendly_goal()));

		// figure out how many defenders, offenders etc
		Player::Ptr goalie = players[0];

		std::size_t ndefenders = 1; // includes goalie
		//int noffenders = 0;

		switch (players.size()) {
			case 5:
				//++ndefenders;

			case 4:
				//++noffenders;

			case 3:
				++ndefenders;

			case 2:
				//++noffenders;
				break;
		}

		std::vector<Player::Ptr> defenders; // excludes goalie
		std::vector<Player::Ptr> offenders;
		// start from 1, to exclude goalie
		for (std::size_t i = 1; i < players.size(); ++i) {
			if (i < ndefenders) {
				defenders.push_back(players[i]);
			} else {
				offenders.push_back(players[i]);
			}
		}

		// draw a circle of radius 50cm from the ball
		Point ball_pos = world.ball().position();

		// calculate angle between robots
		const double delta_angle = AVOIDANCE_ANGLE + separation_angle * M_PI / 180.0;

		// a ray that shoots from the center to friendly goal.
		const Point shoot = Point(-1, 0) * AVOIDANCE_DIST;

		const Point p1 = shoot;
		const Point p2 = shoot.rotate(delta_angle);
		const Point p3 = shoot.rotate(-delta_angle);

		switch (offenders.size()) {
			case 3:
				AI::HL::Tactics::free_move(world, offenders[2], p3);

			case 2:
				AI::HL::Tactics::free_move(world, offenders[1], p2);

			case 1:
				AI::HL::Tactics::free_move(world, offenders[0], p1);
				break;

			default:
				// that's too bad
				break;
		}

		// run defender
		defender.set_players(defenders, goalie);
	}

	Strategy::Ptr KickoffEnemyStrategy::create(World &world) {
		const Strategy::Ptr p(new KickoffEnemyStrategy(world));
		return p;
	}

	KickoffEnemyStrategy::KickoffEnemyStrategy(World &world) : Strategy(world), defender(world) {
		world.playtype().signal_changed().connect(sigc::mem_fun(this, &KickoffEnemyStrategy::on_play_type_changed));
	}

	KickoffEnemyStrategy::~KickoffEnemyStrategy() {
	}

	void KickoffEnemyStrategy::on_play_type_changed() {
		if (world.playtype() != PlayType::PREPARE_KICKOFF_ENEMY
				|| world.playtype() != PlayType::EXECUTE_KICKOFF_ENEMY) {
			resign();
		}
	}

	KickoffEnemyStrategyFactory::KickoffEnemyStrategyFactory() : StrategyFactory("Kickoff Enemy", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	KickoffEnemyStrategyFactory::~KickoffEnemyStrategyFactory() {
	}

	Strategy::Ptr KickoffEnemyStrategyFactory::create_strategy(World &world) const {
		return KickoffEnemyStrategy::create(world);
	}
}

