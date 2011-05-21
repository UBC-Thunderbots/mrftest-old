#include "ai/hl/stp/tactic/cm_defense.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/evaluation/cm_evaluation.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "geom/angle.h"
#include "geom/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;
using AI::HL::STP::TCoordinate;

namespace {
	const double DEFENSE_OFF_BALL = 0.09;
	const double MARK_OFF_OPPONENT = 0.270;

	class TDefendLine : public Tactic {
		public:
			TDefendLine(const World &world, TCoordinate _p1, TCoordinate _p2, double _distmin, double _distmax) : Tactic(world), p1(_p1), p2(_p2), distmin(_distmin), distmax(_distmax) {
			}

		private:
			TCoordinate p1, p2;
			double distmin, distmax;

			bool intercepting;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				// return players
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(Point(0, 0)));
			}

			void execute();

			std::string description() const {
				return "defend_line";
			}
	};

	class TDefendPoint : public Tactic {
		public:
			TDefendPoint(const World &world, TCoordinate _center, double _distmin, double _distmax) : Tactic(world), center(_center), distmin(_distmin), distmax(_distmax) {}

		private:
			TCoordinate center;

			double distmin, distmax;

			bool intercepting;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(Point(0, 0)));
			}

			void execute();

			std::string description() const {
				return "defend_point";
			}
	};

	class TDefendLane : public Tactic {
		public:
			TDefendLane(const World &world, TCoordinate _p1, TCoordinate _p2) : Tactic(world), p1(_p1), p2(_p2) {}

		private:
			TCoordinate p1, p2;
			bool intercepting;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(Point(0, 0)));
			}

			void execute();

			std::string description() const {
				return "defend_lane";
			}
	};
	/*
	   class TBlock : public Tactic {
	    public:
	        bool intercepting;

	        TBlock(const World &world, double _distmin, double _distmax, int _prefside) : Tactic(world), distmin(_distmin), distmax(_distmax), prefside(_prefside) {}

	    private:
	        double distmin, distmax;
	        int prefside;

	        Point pref_point;
	        bool pref_point_set;

	        Player::Ptr select(const std::set<Player::Ptr> &players) const;

	        void execute();

	        std::string description() const {
	            return "block";
	        }
	   };

	   class TMark : public Tactic {
	    public:
	        enum Type { FromBall, FromOurGoal, FromTheirGoal, FromShot };
	        TMark(const World &world, int _target, Type _type) : Tactic(world), target(_target), type(_type) {}

	    private:
	        int target;
	        Type type;

	        Player::Ptr select(const std::set<Player::Ptr> &players) const;

	        void execute();

	        std::string description() const {
	            return "mark";
	        }
	   };
	 */
}

void TDefendLine::execute() {
	intercepting = true;

	Point ball = world.ball().position();

	Point target, velocity;
	double angle;

	Point v[2] = { p1.as_vector(world), p2.as_vector(world) };

	// Position
	if (!Evaluation::CMEvaluation::defend_line(world, LATENCY_DELAY, v[0], v[1], distmin, distmax, DEFENSE_OFF_BALL, intercepting, target, velocity)) {
		target = ball;
		velocity = Point(0, 0);
	}

	// Obstacles... don't avoid the ball if away from the line.
	// int obs_flags = OBS_EVERYTHING_BUT_ME(me);

	Point mypos = player->position();

	// if (distance_to_line(v[0], v[1], mypos) < distance_to_line(v[0], v[1], ball))
	// obs_flags &= ~OBS_BALL;

	// target = Evaluation::CMEvaluation::find_open_position_and_yield(world, target, (v[0] + v[1]) / 2.0, /*getObsFlags() |*/ OBS_OPPONENTS);

	// Angle
	// if (world.teammate_type(me) == ROBOT_TYPE_DIFF)
	// angle = world.teammate_nearest_direction(me, (v[0] - v[1]).orientation());
	// else
	angle = (ball - target).orientation();

	player->move(target, angle, velocity);
	// Action::move(world, player, (p1+p2)/2);
}

void TDefendPoint::execute() {
	intercepting = true;

	Point ball = world.ball().position();
	Point centerv = center.as_vector(world);

	Point target, velocity;
	double angle;

	// Position
	if (!Evaluation::CMEvaluation::defend_point(world, LATENCY_DELAY, centerv, distmin, distmax, DEFENSE_OFF_BALL, intercepting, target, velocity)) {
		target = ball;
		velocity = Point(0, 0);
	}

	// Angle
	angle = (target - centerv).orientation();

	// if (world.teammate_type(me) == ROBOT_TYPE_DIFF)
	// angle = world.teammate_nearest_direction(me, angle_mod(angle + M_2_PI));

	// Obstacles... don't avoid the ball if away from the point.
	// int obs_flags = OBS_EVERYTHING_BUT_ME(me);

	Point mypos = player->position();

	// if ((mypos - centerv).dot(ball - mypos) > 0)
	// obs_flags &= ~OBS_BALL;

	// target = Evaluation::CMEvaluation::find_open_position_and_yield(world, target, centerv, /*getObsFlags() |*/ OBS_OPPONENTS);

	player->move(target, angle, velocity);
	// Action::move(world, player, center);
}

void TDefendLane::execute() {
	intercepting = true;

	Point target, velocity;
	double angle;

	Point v0 = p1.as_vector(world);
	Point v1 = p2.as_vector(world);

	Evaluation::CMEvaluation::defend_on_line(world, LATENCY_DELAY, v0, v1, intercepting, target, velocity);

	/*if (world.teammate_type(me) == ROBOT_TYPE_DIFF) {
	        angle = (v0 - v1).orientation();
	        angle = world.teammate_nearest_direction(me, angle);
	   } else*/angle = (world.ball().position() - target).orientation();

	Point opt0, opt1;
	// opt0 = Evaluation::CMEvaluation::find_open_position_and_yield(world, target, v0, /*getObsFlags() |*/ OBS_OPPONENTS);
	// opt1 = Evaluation::CMEvaluation::find_open_position_and_yield(world, target, v1, /*getObsFlags() |*/ OBS_OPPONENTS);
	target = ((target - opt0).len() < (target - opt1).len()) ? opt0 : opt1;

	player->move(target, angle, velocity);
	// Action::move(world, player, (p1+p2)/2);
}

/*
   void TBlock::execute() {
    //Action::move(world, player, (p1+p2)/2);
   }

   void TMark::execute() {
    //Action::move(world, player, (p1+p2)/2);
   }
 */

Tactic::Ptr AI::HL::STP::Tactic::tdefend_line(const World &world, TCoordinate _p1, TCoordinate _p2, double _distmin, double _distmax) {
	Tactic::Ptr p(new TDefendLine(world, _p1, _p2, _distmin, _distmax));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefend_point(const World &world, TCoordinate _center, double _distmin, double _distmax) {
	Tactic::Ptr p(new TDefendPoint(world, _center, _distmin, _distmax));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefend_lane(const World &world, TCoordinate _p1, TCoordinate _p2) {
	Tactic::Ptr p(new TDefendLane(world, _p1, _p2));
	return p;
}
/*
   Tactic::Ptr AI::HL::STP::Tactic::tblock(const AI::HL::W::World &world) {
    Tactic::Ptr p(new TBlock(world));
    return p;
   }

   Tactic::Ptr AI::HL::STP::Tactic::tmark(const AI::HL::W::World &world) {
    Tactic::Ptr p(new TMark(world));
    return p;
   }
 */

