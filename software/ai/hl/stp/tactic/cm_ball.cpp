#include "ai/hl/stp/tactic/cm_ball.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/move_spin.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/evaluation/cm_evaluation.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "ai/hl/stp/tactic/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;
using AI::HL::STP::Coordinate;
using AI::HL::STP::Region;

namespace {
	// TShoot
	// The amount we'd prefer to shoot at our previous angle.  If an another
	// at least this much bigger appears we'll switch to that.
	const double SHOOT_AIM_PREF_AMOUNT = 0.01745; // 1 degree
	// 0.1221  7 deg

	// We make sure the angle tolerance is always this big.
	const double SHOOT_MIN_ANGLE_TOLERANCE = 0.1745; // Pi / 16

	// Dribbles to open shot when nothing's open.
	const bool SHOOT_DRIBBLE_IF_NO_SHOT = true;

	// TPass
	// The "width" of our teammate's front for the purpose of aiming a
	// pass.  Some connection to it's dribbler's width.
	const double PASS_TARGET_WIDTH = 0.030;

	class TShoot : public Tactic {
		public:
			TShoot(const World &world) : Tactic(world, true) {}

		private:
			Point prev_target;
			bool prev_target_set;

			bool kicked;
			bool done() const {
				return kicked;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			std::string description() const {
				return "tshoot";
			}
	};

	class TSteal : public Tactic {
		public:
			TSteal(const World &world) : Tactic(world, true) {}

		private:
			bool none;
			bool done() const {
				return none;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			std::string description() const {
				return "tsteal";
			}
	};

	class TClear : public Tactic {
		public:
			TClear(const World &world) : Tactic(world, true) {}

		private:
			Point prev_target;
			bool prev_target_set;
			bool kicked;
			bool done() const {
				return kicked;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			std::string description() const {
				return "tclear";
			}
	};

	class TActiveDef : public Tactic {
		public:
			TActiveDef(const World &world) : Tactic(world, true) {}

		private:
			bool finished;
			bool done() const {
				return finished;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			std::string description() const {
				return "tactive_def";
			}
	};

	class TDribbleToRegion : public Tactic {
		public:
			TDribbleToRegion(const World &world, Region _region) : Tactic(world, true), region(_region) {}

		private:
			Region region;	
			bool done() const {
				return region.inside(player->position());
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			std::string description() const {
				return "tdribble_to_region";
			}
	};

	class TSpinToRegion : public Tactic {
		public:
			TSpinToRegion(const World &world, Region _region) : Tactic(world, true), region(_region) {}

		private:
			Region region;
			bool done() const {
				return region.inside(player->position());
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute();

			std::string description() const {
				return "tspin_to_region";
			}
	};
}

// not too sure how good is their shoot =/
// always aim before shooting now
void TShoot::execute() {
	kicked = false;
	Point ball = world.ball().position();

	Point target;
	double angle_tolerance;

	if (!prev_target_set) {
		prev_target = (ball - (player->position() - ball));
		prev_target_set = true;
	}

	bool got_target = false;

	got_target = Evaluation::CMEvaluation::aim(world, LATENCY_DELAY, world.ball().position(), Point(world.field().length() / 2, -world.field().goal_width() / 2), Point(world.field().length() / 2, world.field().goal_width() / 2), OBS_EVERYTHING_BUT_US, prev_target, SHOOT_AIM_PREF_AMOUNT, target, angle_tolerance);
	if (got_target) {
		if (angle_tolerance < SHOOT_MIN_ANGLE_TOLERANCE) {
			angle_tolerance = SHOOT_MIN_ANGLE_TOLERANCE;
		}

		prev_target = target;

		// Aim for point in front of obstacles.
		Point rtarget;
		Point targ_ball;

		targ_ball = target - ball;
		if (targ_ball.len() > 0.5) {
			targ_ball = targ_ball.norm(0.5);
		}
		Evaluation::obs_line_first(world, target - targ_ball * 0.75, target, OBS_OPPONENTS, rtarget, Robot::MAX_RADIUS);

		Action::dribble(world, player, rtarget);
		kicked = Action::shoot_target(world, player, target);

	} else {
		kicked = Action::shoot_goal(world, player);
	}

	player->flags(0);
}

void TSteal::execute() {
	none = false;
	const EnemyTeam &enemy = world.enemy_team();
	Point dirToBall = (world.ball().position() - player->position()).norm();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (Evaluation::possess_ball(world, enemy.get(i))) {
			Action::move_spin(player, world.ball().position() + Robot::MAX_RADIUS * dirToBall);
			return;
		}
	}
	none = true;

	player->flags(0);
}

void TClear::execute() {
	kicked = false;
	Point ball = world.ball().position();

	Point target;
	double angle_tolerance;

	bool aimed = false;

	Point downfield[2];

	downfield[0] = Point(ball.x + 0.180, -world.field().width() / 2);
	downfield[1] = Point(ball.x + 0.180, world.field().width() / 2);

	if (!prev_target_set) {
		prev_target = world.field().enemy_goal();
	}

	aimed = Evaluation::CMEvaluation::aim(world, LATENCY_DELAY, world.ball().position(), downfield[0], downfield[1], OBS_EVERYTHING_BUT_US, prev_target, SHOOT_AIM_PREF_AMOUNT, target, angle_tolerance);

	if (!aimed) {
		// Guaranteed to return true and fill in the parameters when obs_flags is empty.
		Evaluation::CMEvaluation::aim(world, LATENCY_DELAY, world.ball().position(), downfield[0], downfield[1], 0, target, angle_tolerance);
	}

	target = (target - ball).norm(std::min(world.field().length() / 2 - ball.x, 1.000)) + ball;

	// If the target tolerances include the goal then just aim there.
	double a = (target - ball).orientation();
	double a_to_goal = (world.field().enemy_goal() - ball).orientation();

	if (std::fabs(angle_mod(a - a_to_goal)) < 0.8 * angle_tolerance) {
		if (a > a_to_goal) {
			target = world.field().enemy_goal();
			angle_tolerance -= std::fabs(angle_mod(a - a_to_goal));
		} else if (a < a_to_goal) {
			target = world.field().enemy_goal();
			angle_tolerance -= std::fabs(angle_mod(a - a_to_goal));
		}
	}

	if (angle_tolerance < SHOOT_MIN_ANGLE_TOLERANCE) {
		angle_tolerance = SHOOT_MIN_ANGLE_TOLERANCE;
	}

	prev_target = target;
	prev_target_set = true;

	kicked = Action::shoot_target(world, player, target);
}

void TActiveDef::execute() {
	finished = false;
	const EnemyTeam &enemy = world.enemy_team();
	
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (Evaluation::possess_ball(world, enemy.get(i))) {
			Point dirToBall = (world.ball().position() - enemy.get(i)->position()).norm();
			Action::move_spin(player, world.ball().position() + 0.75 * Robot::MAX_RADIUS * dirToBall);
			return;
		}
	}

	finished = Action::repel(world, player);

	player->flags(0);
}

void TDribbleToRegion::execute() {
	Action::dribble(world, player, region.center_position());
}

void TSpinToRegion::execute() {
	Action::move_spin(player, region.center_position());
}

Tactic::Ptr AI::HL::STP::Tactic::tshoot(const World &world) {
	Tactic::Ptr p(new TShoot(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tsteal(const World &world) {
	Tactic::Ptr p(new TSteal(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tclear(const World &world) {
	Tactic::Ptr p(new TClear(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tactive_def(const World &world) {
	Tactic::Ptr p(new TActiveDef(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdribble_to_region(const World &world, Region _region) {
	Tactic::Ptr p(new TDribbleToRegion(world, _region));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tspin_to_region(const World &world, Region _region) {
	Tactic::Ptr p(new TSpinToRegion(world, _region));
	return p;
}

