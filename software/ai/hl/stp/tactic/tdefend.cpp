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

namespace {
	/**
	 * Goalie in a team of N robots.
	 */
	class TGoalie : public Tactic {
		public:
			TGoalie(const World &world, size_t defender_role) : Tactic(world), defender_role(defender_role) {
			}

		private:
			size_t defender_role;
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
			std::string description() const {
				if (world.friendly_team().size() > defender_role + 1) {
					return "tgoalie";
				} else {
					return "tgoalie lone";
				}
			}
	};

	class TDefender : public Tactic {
		public:
			TDefender(const World &world, unsigned i) : Tactic(world), index(i) {
			}

		private:
			unsigned index;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "tdefender";
			}
	};

	void TGoalie::execute() {
		Point dirToGoal = (world.field().friendly_goal() - world.ball().position()).norm();
		Point target = world.field().friendly_goal() - (2 * Robot::MAX_RADIUS * dirToGoal);
		
		Action::goalie_move(world, player, target);	
	}

	Player::Ptr TDefender::select(const std::set<Player::Ptr> &players) const {
		Point target = Evaluation::evaluate_tdefense(world, player, index);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(target));
	}

	void TDefender::execute() {
		Point diff = world.ball().position() - world.field().friendly_goal();
		if (diff.len() <= 0.9 && index == 1){
			Action::ram(world, player);
			return;
		}
		Point target = Evaluation::evaluate_tdefense(world, player, index);
		if (Evaluation::ball_on_net(world)){ // ball is coming towards net
			if (index == 2) { 
				// 2nd defender should not go after the ball unless the ball is far enough from our goal
				// and on our side of the field
				if (diff.len() > 4 * (index+1) * Robot::MAX_RADIUS && world.ball().position().x < -world.field().centre_circle_radius()){
					Action::ram(world, player);
					return;
				}
			} else if (diff.len() < 4 * (index+2) * Robot::MAX_RADIUS && diff.len() > 4 * (index) * Robot::MAX_RADIUS){ 
				// 1st defender defense 
				Action::ram(world, player);
				return;
			}
			target = calc_block_cone(world.field().friendly_goal_boundary().first, world.field().friendly_goal_boundary().second, world.ball().position(), Robot::MAX_RADIUS);
		}
		Action::defender_move(world, player, target);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::tgoalie(const World &world, const size_t defender_role) {
	const Tactic::Ptr p(new TGoalie(world, defender_role));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefender1(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new TDefender(world, 1));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tdefender2(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new TDefender(world, 2));
	return p;
}

