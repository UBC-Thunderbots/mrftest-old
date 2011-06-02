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
using AI::HL::STP::Coordinate;

namespace {
	const double DEFENSE_OFF_BALL = 0.09;
	const double MARK_OFF_OPPONENT = 0.270;

	class TDefendLine : public Tactic {
		public:
			TDefendLine(const World &world, Coordinate _p1, Coordinate _p2, double _distmin, double _distmax) : Tactic(world), p1(_p1), p2(_p2), distmin(_distmin), distmax(_distmax) {
			}

		private:
			Coordinate p1, p2;
			double distmin, distmax;

			bool intercepting;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>((p1.position() + p2.position()) / 2));
			}

			void execute();

			std::string description() const {
				return "tdefend_line";
			}
	};

	class TDefendPoint : public Tactic {
		public:
			TDefendPoint(const World &world, Coordinate _center, double _distmin, double _distmax) : Tactic(world), center(_center), distmin(_distmin), distmax(_distmax) {}

		private:
			Coordinate center;

			double distmin, distmax;

			bool intercepting;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(center.position()));
			}

			void execute();

			std::string description() const {
				return "tdefend_point";
			}
	};

	class TDefendLane : public Tactic {
		public:
			TDefendLane(const World &world, Coordinate _p1, Coordinate _p2) : Tactic(world), p1(_p1), p2(_p2) {}

		private:
			Coordinate p1, p2;
			bool intercepting;

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>((p1.position() + p2.position()) / 2));
			}

			void execute();

			std::string description() const {
				return "tdefend_lane";
			}
	};
}

void TDefendLine::execute() {
	intercepting = true;

	Point ball = world.ball().position();

	Point target, velocity;
	double angle;

	Point v[2] = { p1.position(), p2.position() };

	// Position
	if (!Evaluation::CMEvaluation::defend_line(world, LATENCY_DELAY, v[0], v[1], distmin, distmax, DEFENSE_OFF_BALL, intercepting, target, velocity)) {
		target = ball;
		velocity = Point(0, 0);
	}

	// we don't seem to have an equivalent of obstacle avoidance flag to set for player->move in our ai

	Point mypos = player->position();

	// cm getObsFlags seems to check for priority, so should be handled by setting priority in our code
	target = Evaluation::CMEvaluation::find_open_position_and_yield(world, target, (v[0] + v[1]) / 2.0, 0 | OBS_OPPONENTS);

	// Angle
	angle = (ball - target).orientation();

	player->move(target, angle, velocity);
	// Action::move(world, player, (p1+p2)/2);
}

void TDefendPoint::execute() {
	intercepting = true;

	Point ball = world.ball().position();
	Point centerv = center.position();

	Point target, velocity;
	double angle;

	// Position
	if (!Evaluation::CMEvaluation::defend_point(world, LATENCY_DELAY, centerv, distmin, distmax, DEFENSE_OFF_BALL, intercepting, target, velocity)) {
		target = ball;
		velocity = Point(0, 0);
	}

	Point mypos = player->position();

	target = Evaluation::CMEvaluation::find_open_position_and_yield(world, target, centerv, 0 | OBS_OPPONENTS);

	// Angle
	angle = (target - centerv).orientation();

	player->move(target, angle, velocity);
}

void TDefendLane::execute() {
	intercepting = true;

	Point target, velocity;
	double angle;

	Point v0 = p1.position();
	Point v1 = p2.position();

	Evaluation::CMEvaluation::defend_on_line(world, LATENCY_DELAY, v0, v1, intercepting, target, velocity);

	Point opt0 = Evaluation::CMEvaluation::find_open_position_and_yield(world, target, v0, 0 | OBS_OPPONENTS);
	Point opt1 = Evaluation::CMEvaluation::find_open_position_and_yield(world, target, v1, 0 | OBS_OPPONENTS);

	target = ((target - opt0).len() < (target - opt1).len()) ? opt0 : opt1;

	angle = (world.ball().position() - target).orientation();

	player->move(target, angle, velocity);
}

Tactic::Ptr AI::HL::STP::Tactic::tdefend_line(const World &world, Coordinate _p1, Coordinate _p2, double _distmin, double _distmax) {
	Tactic::Ptr p(new TDefendLine(world, _p1, _p2, _distmin, _distmax));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefend_point(const World &world, Coordinate _center, double _distmin, double _distmax) {
	Tactic::Ptr p(new TDefendPoint(world, _center, _distmin, _distmax));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefend_lane(const World &world, Coordinate _p1, Coordinate _p2) {
	Tactic::Ptr p(new TDefendLane(world, _p1, _p2));
	return p;
}

