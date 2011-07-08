#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/tdefend.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/defend.h"
#include "ai/hl/util.h"

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
		Point target = Evaluation::evaluate_tdefense(world, player, index);
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

