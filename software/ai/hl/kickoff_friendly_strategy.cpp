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
	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const double AVOIDANCE_ANGLE = 2.0 * std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST);

	DoubleParam separation_angle("kickoff: angle to separate players (degrees)", 40, 0, 80);

	/**
	 * Manages the robots during a stoppage in place (that is, when the game is in PlayType::STOP).
	 */
	class KickoffFriendlyStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;

			/**
			 * Creates a new KickoffFriendlyStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			KickoffFriendlyStrategy(World &world);
			~KickoffFriendlyStrategy();

			void on_player_added(std::size_t);
			void on_player_removed();

			void prepare_kickoff_friendly();
			void execute_kickoff_friendly();
			
			void prepare();
			void execute();

			/**
			 * Dynamic assignment every tick can be bad if players keep changing the roles.
			 */
			void run_assignment();

			// the players invovled
			std::vector<Player::Ptr> defenders;
			std::vector<Player::Ptr> offenders;
			Player::Ptr kicker;

			AI::HL::Defender defender;
			AI::HL::Offender offender;
	};

	/**
	 * A factory for constructing \ref KickoffFriendlyStrategy "KickoffFriendlyStrategies".
	 */
	class KickoffFriendlyStrategyFactory : public StrategyFactory {
		public:
			KickoffFriendlyStrategyFactory();
			~KickoffFriendlyStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of KickoffFriendlyStrategyFactory.
	 */
	KickoffFriendlyStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::PREPARE_KICKOFF_FRIENDLY,
		PlayType::EXECUTE_KICKOFF_FRIENDLY,
	};

	StrategyFactory &KickoffFriendlyStrategy::factory() const {
		return factory_instance;
	}

	void KickoffFriendlyStrategy::prepare_kickoff_friendly() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		prepare();		
	}

	void KickoffFriendlyStrategy::execute_kickoff_friendly() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		//prepare();

		execute();
	}

	void KickoffFriendlyStrategy::prepare() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		// draw a circle of radius 50cm from the ball
		Point ball_pos = world.ball().position();

		// a ray that shoots from the center to friendly goal.
		const Point shoot = Point(-1, 0) * AVOIDANCE_DIST;

		// do matching
		std::vector<Point> positions;

		switch (offenders.size()) {
			case 2:
				positions.push_back(Point(-AVOIDANCE_DIST, 5 * Robot::MAX_RADIUS));

			case 1:
				positions.push_back(Point(-AVOIDANCE_DIST, -5 * Robot::MAX_RADIUS));

			default:
				break;
		}

		AI::HL::Util::waypoints_matching(offenders, positions);
		for (std::size_t i = 0; i < offenders.size(); ++i) {
			AI::HL::Tactics::free_move(world, offenders[i], positions[i]);
		}


		defender.set_chase(false);
		defender.tick();

		if (kicker.is()) AI::HL::Tactics::free_move(world, kicker, shoot);
		
	}

	void KickoffFriendlyStrategy::execute(){
		int best = AI::HL::Util::choose_best_pass(world, offenders);
		
		if (kicker.is()) AI::HL::Tactics::shoot(world, kicker, AI::Flags::FLAG_AVOID_BALL_TINY, offenders[best]->position());
		offender.set_players(offenders);
		offender.tick();
	}

	void KickoffFriendlyStrategy::run_assignment() {
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
		kicker.reset();

		// start from 1, to exclude goalie
		for (std::size_t i = 1; i < players.size(); ++i) {
			if (i < ndefenders) {
				defenders.push_back(players[i]);
			} else if (!kicker.is()) {
				kicker = players[i];
			} else {
				offenders.push_back(players[i]);
			}
		}

		LOG_INFO(Glib::ustring::compose("player reassignment %1 defenders, %2 offenders", ndefenders, offenders.size()));

		defender.set_players(defenders, goalie);
	}

	Strategy::Ptr KickoffFriendlyStrategy::create(World &world) {
		const Strategy::Ptr p(new KickoffFriendlyStrategy(world));
		return p;
	}

	KickoffFriendlyStrategy::KickoffFriendlyStrategy(World &world) : Strategy(world), defender(world), offender(world) {
		world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &KickoffFriendlyStrategy::on_player_added));
		world.friendly_team().signal_robot_removed().connect(sigc::mem_fun(this, &KickoffFriendlyStrategy::on_player_removed));
		run_assignment();
	}

	void KickoffFriendlyStrategy::on_player_added(std::size_t) {
		run_assignment();
	}

	void KickoffFriendlyStrategy::on_player_removed() {
		run_assignment();
	}

	KickoffFriendlyStrategy::~KickoffFriendlyStrategy() {
	}

	KickoffFriendlyStrategyFactory::KickoffFriendlyStrategyFactory() : StrategyFactory("Kickoff Friendly", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	KickoffFriendlyStrategyFactory::~KickoffFriendlyStrategyFactory() {
	}

	Strategy::Ptr KickoffFriendlyStrategyFactory::create_strategy(World &world) const {
		return KickoffFriendlyStrategy::create(world);
	}
}

