#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/defend.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/util.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;

namespace {

	BoolParam tdefend("Whether or not Terence Defense should take the place of normal defense", "STP/Tactic/defend",  false);

	/**
	 * Goalie in a team of N robots.
	 */
	class Goalie2 : public Tactic {
		public:
			Goalie2(const World &world, size_t defender_role) : Tactic(world), defender_role(defender_role) {
			}

		private:
			size_t defender_role;
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
			std::string description() const {
				if (world.friendly_team().size() > defender_role + 1) {
					return "goalie-dynamic duo";
				} else {
					return "goalie-dynamic lone";
				}
			}
	};

	/**
	 * Goalie in a team of N robots.
	 */
	class Goalie : public Tactic {
		public:
			Goalie(const World &world) : Tactic(world) {
			}

		private:
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
			std::string description() const {
				return "goalie (helped by defender)";
			}
	};

	class Defender : public Tactic {
		public:
			Defender(const World &world, unsigned i) : Tactic(world), index(i) {
			}

		private:
			unsigned index;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "extra defender";
			}
	};

	void Goalie2::execute() {
		if (tdefend){
			Point dirToGoal = (world.field().friendly_goal() - world.ball().position()).norm();
			Point dest = world.field().friendly_goal() - (2 * Robot::MAX_RADIUS * dirToGoal);
			Action::goalie_move(world, player, dest);
		} else if (world.friendly_team().size() > defender_role + 1) {
			// has defender
			auto waypoints = Evaluation::evaluate_defense(world);
			Action::goalie_move(world, player, waypoints[0]);
		} else {
			// solo
			AI::HL::STP::Action::lone_goalie(world, player);
		}
	}

	void Goalie::execute() {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[0];
		if (tdefend) {
			Point dirToGoal = (world.field().friendly_goal() - world.ball().position()).norm();
			dest = world.field().friendly_goal() - (2 * Robot::MAX_RADIUS * dirToGoal);
		}
		Action::goalie_move(world, player, dest);
	}

	Player::Ptr Defender::select(const std::set<Player::Ptr> &players) const {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[index];
		if (tdefend && index > 0 && index < 3) dest = Evaluation::evaluate_tdefense(world, player, index);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
	}

	void Defender::execute() {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[index];
		if (tdefend && index > 0 && index < 3) {
			Point diff = world.ball().position() - world.field().friendly_goal();
			if (diff.len() <= 0.9 && index == 1){
				Action::repel(world, player);
				return;
			}
			dest = Evaluation::evaluate_tdefense(world, player, index);
			if (Evaluation::ball_on_net(world)){ // ball is coming towards net
				if (index == 2) { 
					// 2nd defender should not go after the ball unless the ball is far enough from our goal
					// and on our side of the field
					if (diff.len() > 4 * (index+1) * Robot::MAX_RADIUS && world.ball().position().x < -world.field().centre_circle_radius()){
						Action::repel(world, player);
						return;
					}
				} else if (diff.len() < 4 * (index+2) * Robot::MAX_RADIUS && diff.len() > 4 * (index) * Robot::MAX_RADIUS){ 
					// 1st defender defense 
					Action::repel(world, player);
					return;
				}
				
			}
		}
		Action::defender_move(world, player, dest);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::goalie_dynamic(const World &world, const size_t defender_role) {
	const Tactic::Ptr p(new Goalie2(world, defender_role));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_goalie(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Goalie(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_defender(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Defender(world, 1));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra1(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Defender(world, 2));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra2(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Defender(world, 3));
	return p;
}

