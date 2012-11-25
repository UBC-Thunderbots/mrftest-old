#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/tdefend.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/defend.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/util.h"
#include "geom/util.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;
using AI::HL::STP::Coordinate;

namespace {
	/**
	 * Goalie in a team of N robots.
	 */
	class TGoalie : public Tactic {
		public:
			TGoalie(World world, size_t defender_role) : Tactic(world), defender_role(defender_role) {
			}

		private:
			size_t defender_role;
			void execute();
			Player select(const std::set<Player> &) const {
				assert(0);
			}
			Glib::ustring description() const {
				if (world.friendly_team().size() > defender_role + 1) {
					return "tgoalie";
				} else {
					return "tgoalie lone";
				}
			}
	};

	class TDefender : public Tactic {
		public:
			TDefender(World world, unsigned i) : Tactic(world), index(i) {
			}

		private:
			unsigned index;
			Player select(const std::set<Player> &players) const;
			void execute();
			Glib::ustring description() const {
				return "tdefender";
			}
	};

	void TGoalie::execute() {
		// Point dirToGoal = (world.field().friendly_goal() - world.ball().position()).norm();
		// Point target = world.field().friendly_goal() - (2 * Robot::MAX_RADIUS * dirToGoal);

		// Action::goalie_move(world, player, target);
		Action::lone_goalie(world, player);
	}

	Player TDefender::select(const std::set<Player> &players) const {
		Point target = Evaluation::evaluate_tdefense(world, index);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(target));
	}

	void TDefender::execute() {
		Point target = Evaluation::evaluate_tdefense(world, index);
		if (Evaluation::ball_on_net(world)) { // ball is coming towards net
			auto waypoints = Evaluation::evaluate_defense();
			target = waypoints[index];
		}
		Action::defender_move(world, player, target);
	}
	
	class TDefendLine : public Tactic {
		public:
			TDefendLine(World world, Coordinate p1_, Coordinate p2_, double dist_min_, double dist_max_) : Tactic(world), p1(p1_), p2(p2_), dist_min(dist_min_), dist_max(dist_max_) {
			}

		private:
			Coordinate p1, p2;
			double dist_min, dist_max;

			Player select(const std::set<Player> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>((p1.position() + p2.position()) / 2));
			}

			void execute();

			Glib::ustring description() const {
				return "tdefend_line";
			}
	};

}

void TDefendLine::execute() {

	Point ball = world.ball().position();

	Point target, velocity;
	Angle angle;

	Point v[2] = { p1.position(), p2.position() }; 
	velocity = Point(0, 0);

	//Point mypos = player.position();

	target = Evaluation::evaluate_tdefense_line(world, v[0], v[1], dist_min, dist_max);

	// Angle
	angle = (ball - target).orientation();

	//player.move(target, angle, velocity);
	Action::defender_move(world, player, target);
}

Tactic::Ptr AI::HL::STP::Tactic::tgoalie(World world, const size_t defender_role) {
	Tactic::Ptr p(new TGoalie(world, defender_role));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefender1(AI::HL::W::World world) {
	Tactic::Ptr p(new TDefender(world, 1));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefender2(AI::HL::W::World world) {
	Tactic::Ptr p(new TDefender(world, 2));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefend_line(World world, Coordinate p1_, Coordinate p2_, double dist_min_, double dist_max_) {
	Tactic::Ptr p(new TDefendLine(world, p1_, p2_, dist_min_, dist_max_));
	return p;
}

