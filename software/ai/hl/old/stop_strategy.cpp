#include "ai/flags.h"
#include "ai/hl/old/strategy.h"
#include "ai/hl/old/tactics.h"
#include "ai/hl/util.h"
#include "uicomponents/param.h"
#include <vector>

using AI::HL::Strategy;
using AI::HL::StrategyFactory;
using namespace AI::HL::W;

namespace {
#warning find better home for this variable

	// The closest distance players allowed to the ball
	// DO NOT make this EXACT, instead, add a little tolerance!
	const double AVOIDANCE_DIST = 0.50 + Robot::MAX_RADIUS + Ball::RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const double AVOIDANCE_ANGLE = 2.0 * asin(Robot::MAX_RADIUS / AVOIDANCE_DIST);

	DoubleParam separation_angle("stop: angle to separate players (degrees)", 20, 0, 90);

	/**
	 * Manages the robots during a stoppage in place (that is, when the game is in PlayType::STOP).
	 */
	class StopStrategy : public Strategy {
		public:
			StrategyFactory &factory() const;
			void stop();

			/**
			 * Creates a new StopStrategy.
			 *
			 * \param[in] world the World in which to operate.
			 */
			static Strategy::Ptr create(World &world);

		private:
			void execute_indirect_free_kick_enemy();
			void execute_direct_free_kick_enemy();

			StopStrategy(World &world);
			~StopStrategy();
			// bool valid(Point p) const;
	};

	/**
	 * A factory for constructing \ref StopStrategy "StopStrategies".
	 */
	class StopStrategyFactory : public StrategyFactory {
		public:
			StopStrategyFactory();
			~StopStrategyFactory();
			Strategy::Ptr create_strategy(World &world) const;
	};

	/**
	 * The global instance of StopStrategyFactory.
	 */
	StopStrategyFactory factory_instance;

	/**
	 * The play types handled by this strategy.
	 */
	const PlayType::PlayType HANDLED_PLAY_TYPES[] = {
		PlayType::STOP,
		PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY,
		PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY,
	};

	StrategyFactory &StopStrategy::factory() const {
		return factory_instance;
	}

	/*
	   bool StopStrategy::valid(Point p) const {
	    // cannot be too far away in x
	    if (std::fabs(p.x) > world.field().length() / 2) {
	        return false;
	    }
	    // cannot be too far away in y
	    if (std::fabs(p.y) > world.field().width() / 2) {
	        return false;
	    }

	    if (AI::HL::Util::point_in_friendly_defense(world.field(), p)) {
	        return false;
	    }

	    // TODO: check if point in enemy defense

	    // cannot be too close to ball
	    if ((world.ball().position() - p).len() < AVOIDANCE_MIN) {
	        return false;
	    }

	    return true;
	   }
	 */

	void StopStrategy::execute_direct_free_kick_enemy() {
		stop();
	}

	void StopStrategy::execute_indirect_free_kick_enemy() {
		stop();
	}

	void StopStrategy::stop() {
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
		if (players.size() == 0) {
			return;
		}

		// draw a circle of radius 50cm from the ball
		Point ball_pos = world.ball().position();

		Point start;

		Point goal_pos = world.field().friendly_goal();

		int n_defenders = 0;
		Point defender_pos;

		if (AI::HL::Util::point_in_friendly_defense(world.field(), ball_pos)) {
			// if the goal is inside the circle,
			// then shoot the ray from the enemy goal

			start = Point(ball_pos.x + AVOIDANCE_DIST, ball_pos.y);
		} else {
			// otherwise
			// draw a ray from the center of friendly goal to the ball,
			// and the intersection shall be the start point.

			Point ray = (goal_pos - ball_pos).norm();
			start = ball_pos + ray * AVOIDANCE_DIST;

			// we may even want a defender
			if (players.size() > 2) {
				defender_pos = (ball_pos + goal_pos) * 0.5;
				// if (valid(defender_pos)) {
				n_defenders = 1;
				// }
			}
		}

		std::vector<Player::Ptr> offenders;
		for (std::size_t i = 1 + n_defenders; i < players.size(); ++i) {
			offenders.push_back(players[i]);
		}

		// calculate angle between robots
		const double delta_angle = AVOIDANCE_ANGLE + separation_angle * M_PI / 180.0;

		const Point shoot = (start - ball_pos);

		// create intervals from the start point
		// place players with the interval points
		unsigned int flags = AI::Flags::FLAG_AVOID_BALL_STOP | AI::Flags::calc_flags(world.playtype());

		// the parity determines left or right
		// we only want one of angle = 0, so start at w = 1
		std::vector<Point> positions;
		int w = 1;
		for (std::size_t i = 0; i < offenders.size(); ++i) {
			// bool okay = false;
			// Point p;
			// do {
			double angle = delta_angle * (w / 2) * ((w % 2) ? 1 : -1);
			Point p = ball_pos + shoot.rotate(angle);
			++w;
			// okay = valid(p);
			// } while (!okay);
			positions.push_back(p);
		}

		AI::HL::Util::waypoints_matching(offenders, positions);
		for (std::size_t i = 0; i < offenders.size(); ++i) {
			offenders[i]->move(positions[i], (world.ball().position() - offenders[i]->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
		}

		// player 1 is defender
		if (n_defenders) {
			players[1]->move(defender_pos, (world.ball().position() - players[1]->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
		}

		// TODO: looks fine but should check again
		// maybe: goalie always touching the goal line, and at a point closest to the ball.
		unsigned int goalie_flags = 0;
		Point goalie_pos = world.field().friendly_goal();
		players[0]->move(goalie_pos, (world.ball().position() - players[0]->position()).orientation(), goalie_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}

	Strategy::Ptr StopStrategy::create(World &world) {
		const Strategy::Ptr p(new StopStrategy(world));
		return p;
	}

	StopStrategy::StopStrategy(World &world) : Strategy(world) {
	}

	StopStrategy::~StopStrategy() {
	}

	StopStrategyFactory::StopStrategyFactory() : StrategyFactory("Stop", HANDLED_PLAY_TYPES, G_N_ELEMENTS(HANDLED_PLAY_TYPES)) {
	}

	StopStrategyFactory::~StopStrategyFactory() {
	}

	Strategy::Ptr StopStrategyFactory::create_strategy(World &world) const {
		return StopStrategy::create(world);
	}
}

