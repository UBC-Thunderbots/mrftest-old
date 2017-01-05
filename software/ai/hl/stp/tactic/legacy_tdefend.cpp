#include "ai/hl/stp/tactic/legacy_defend.h"
#include "ai/hl/stp/tactic/legacy_tactic.h"
#include "ai/hl/stp/tactic/legacy_tdefend.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/action/legacy_goalie.h"
#include "ai/hl/stp/action/legacy_defend.h"
#include "ai/hl/stp/action/legacy_repel.h"
#include "ai/hl/stp/action/legacy_ram.h"
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
	class TGoalie final : public LegacyTactic {
		public:
			explicit TGoalie(World world, size_t defender_role) : LegacyTactic(world), defender_role(defender_role) {
			}

		private:
			size_t defender_role;
			void execute() override;
			Player select(const std::set<Player> &) const override {
				assert(false);
			}
			Glib::ustring description() const override {
				if (world.friendly_team().size() > defender_role + 1) {
					return u8"tgoalie";
				} else {
					return u8"tgoalie lone";
				}
			}
	};

	class TDefender final : public LegacyTactic {
		public:
			explicit TDefender(World world, unsigned i, bool active_baller) : LegacyTactic(world), index(i), active_baller(active_baller) {
			}

		private:
			unsigned index;
			bool active_baller;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override {
				return u8"tdefender";
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
		Action::defender_move(world, player, target, active_baller);
	}
	
	class TDefendLine final : public LegacyTactic {
		public:
			explicit TDefendLine(World world, Coordinate p1_, Coordinate p2_, double dist_min_, double dist_max_) : LegacyTactic(world), p1(p1_), p2(p2_), dist_min(dist_min_), dist_max(dist_max_) {
			}

		private:
			Coordinate p1, p2;
			double dist_min, dist_max;

			Player select(const std::set<Player> &players) const override {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>((p1.position() + p2.position()) / 2));
			}

			void execute() override;

			Glib::ustring description() const override {
				return u8"tdefend_line";
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
}

Tactic::Ptr AI::HL::STP::Tactic::tgoalie(World world, const size_t defender_role) {
	Tactic::Ptr p(new TGoalie(world, defender_role));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefender1(AI::HL::W::World world, bool active_baller) {
	Tactic::Ptr p(new TDefender(world, 1, active_baller));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefender2(AI::HL::W::World world, bool active_baller) {
	Tactic::Ptr p(new TDefender(world, 2, active_baller));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefender3(AI::HL::W::World world, bool active_baller) {
	Tactic::Ptr p(new TDefender(world, 3, active_baller));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefend_line(World world, Coordinate p1_, Coordinate p2_, double dist_min_, double dist_max_) {
	Tactic::Ptr p(new TDefendLine(world, p1_, p2_, dist_min_, dist_max_));
	return p;
}
