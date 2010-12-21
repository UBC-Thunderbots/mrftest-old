#include "ai/flags.h"
#include "ai/hl/defender.h"
#include "ai/hl/offender.h"
#include "ai/hl/strategy.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"
#include <algorithm>
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {
	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const double AVOIDANCE_ANGLE = 2.0 * std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST);

	// distance for the offenders to be positioned away from the kicker
	const double SEPERATION_DIST = 10 * Robot::MAX_RADIUS;

	DoubleParam separation_angle("kickoff: angle to separate players (degrees)", 40, 0, 80);

	// hard coded positions for the kicker, and 2 offenders
	Point kicker_position = Point(-0.5 - Ball::RADIUS - Robot::MAX_RADIUS, 0);
	Point ready_positions[2] = { Point(-AVOIDANCE_DIST, -SEPERATION_DIST), Point(-AVOIDANCE_DIST, SEPERATION_DIST) };

	/**
	 * Manages the robots during a kickoff.
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

			/**
			 * Prepares the players into ready positions.
			 */
			void prepare();

			void execute();

			/**
			 * Dynamic assignment every tick can be bad if players keep changing the roles.
			 */
			void run_assignment();
			int cal_partidx();

			bool prepared;

			// the players invovled
			std::vector<Player::Ptr> defenders;
			std::vector<Player::Ptr> offenders;
			Player::Ptr kicker;

			AI::HL::Defender defender;

			int pidx;
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

		if (!prepared) {
			prepare();
		}

		defender.set_chase(false);
		defender.tick();
	}

	void KickoffFriendlyStrategy::execute_kickoff_friendly() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		if (!prepared) {
			prepare();
		} else {
			execute();
		}

		defender.set_chase(false);
		defender.tick();
	}

	void KickoffFriendlyStrategy::prepare() {
		if (world.friendly_team().size() == 0) {
			return;
		}

		if (!kicker.is()) {
			LOG_ERROR("can't have no kicker");
			return;
		}

		if (prepared) {
			return;
		}

		// draw a circle of radius 50cm from the ball
		Point ball_pos = world.ball().position();

		// a ray that shoots from the center to friendly goal.
		std::vector<Point> positions = std::vector<Point>(ready_positions, ready_positions + G_N_ELEMENTS(ready_positions));

		AI::HL::Util::waypoints_matching(offenders, positions);

		for (std::size_t i = 0; i < offenders.size(); ++i) {
			AI::HL::Tactics::free_move(world, offenders[i], positions[i]);
		}

		if (kicker.is()) {
			AI::HL::Tactics::free_move(world, kicker, kicker_position);
		}

		// check if done with prepartion
		prepared = true;

		if ((kicker->position() - kicker_position).len() > AI::HL::Util::POS_CLOSE) {
			prepared = false;
		}

		for (std::size_t i = 0; i < offenders.size(); ++i) {
			if ((offenders[i]->position() - positions[i]).len() > AI::HL::Util::POS_CLOSE) {
				prepared = false;
			}
		}
	}

	void KickoffFriendlyStrategy::execute() {
		// default is for kicker to just shoot forward
		if (kicker.is() && pidx >= 0) {
			if (pidx == 0 && offenders.size() == 1) {
				AI::HL::Tactics::shoot(world, kicker, AI::Flags::FLAG_CLIP_PLAY_AREA, offenders[0]->position());
			} else if (pidx == 2 && offenders.size() == 2) {
				AI::HL::Tactics::shoot(world, kicker, AI::Flags::FLAG_CLIP_PLAY_AREA, offenders[1]->position());
			} else {
				AI::HL::Tactics::shoot(world, kicker, AI::Flags::FLAG_CLIP_PLAY_AREA);
			}
		} else if (kicker.is()) {
			AI::HL::Tactics::shoot(world, kicker, AI::Flags::FLAG_CLIP_PLAY_AREA);
		}
	}

	void KickoffFriendlyStrategy::run_assignment() {
		prepared = false;

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

	int KickoffFriendlyStrategy::cal_partidx() {
		// TODO Find a side of the field to attack
		// choose_best_pass doesn't seem to be able to find one offender to pass to with the current positioning !!
		// best = -1 causes seg fault
		// int best = AI::HL::Util::choose_best_pass(world, offenders);

		// check how the enemy robots are positioned
		// look for some part of the enemy's field that seems vulnerable to attack
		// divide the enemy field into three parts: top, center, and bottom (may try dividing to 2~4 parts??)
		// if the enemy's field seems equally defended, randomly attack a part

		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
		std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().enemy_goal()));
		// check for which part is more vulnerable, break ties by seeing which part is not defended in depth
		std::vector<Robot::Ptr> parts[3];

		// top = 0, center = 1, bottom = 2
		// ignore enemy goalie or not?
		for (std::size_t i = 1; i < enemies.size(); ++i) {
			Robot::Ptr enemy = enemies[i];

			if (enemies[i]->position().y <= world.field().width() / 2 && enemies[i]->position().y >= world.field().width() / 6) {
				parts[0].push_back(enemy);
			} else if (enemies[i]->position().y <= world.field().width() / 6 && enemies[i]->position().y >= -world.field().width() / 6) {
				parts[1].push_back(enemy);
			} else {
				parts[2].push_back(enemy);
			}
		}

		int partidx = -1;
		if (parts[0].size() > parts[1].size() && parts[0].size() > parts[2].size()) {
			partidx = 0;
		} else if (parts[1].size() > parts[0].size() && parts[1].size() > parts[2].size()) {
			partidx = 1;
		} else if (parts[2].size() > parts[0].size() && parts[2].size() > parts[1].size()) {
			partidx = 2;
		}

		// check in depth by seeing which side has more robots behind their first robot closest to the center of the field
		std::size_t depth[3];
		if (partidx == -1) {
			for (std::size_t i = 0; i < 3; ++i) {
				depth[i] = 0;
				double closest = 0;
				for (std::size_t j = 0; i < parts[i].size(); ++j) {
					Robot::Ptr r = parts[i][j];
					closest = std::min(closest, r->position().x);
				}
				for (std::size_t j = 0; i < parts[i].size(); ++j) {
					Robot::Ptr r = parts[i][j];
					if (r->position().x > closest) {
						depth[i]++;
					}
				}
			}
			if (depth[0] > depth[1] && depth[0] > depth[1]) {
				partidx = 0;
			} else if (depth[0] > depth[1] && depth[0] > depth[1]) {
				partidx = 1;
			} else if (depth[0] > depth[1] && depth[0] > depth[1]) {
				partidx = 2;
			}
		}

		// randomly picking a side to shoot to using std::rand()
		if (partidx == -1) {
			partidx = std::rand() % 3;
		}
		return partidx;
	}

	Strategy::Ptr KickoffFriendlyStrategy::create(World &world) {
		const Strategy::Ptr p(new KickoffFriendlyStrategy(world));
		return p;
	}

	KickoffFriendlyStrategy::KickoffFriendlyStrategy(World &world) : Strategy(world), defender(world) {
		world.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &KickoffFriendlyStrategy::on_player_added));
		world.friendly_team().signal_robot_removed().connect(sigc::mem_fun(this, &KickoffFriendlyStrategy::on_player_removed));
		run_assignment();
		pidx = cal_partidx();
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

