#include "ai/hl/stp/tactic/tri_attack.h"
#include "ai/hl/stp/evaluation/tri_attack.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/predicates.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Predicates;
namespace Action = AI::HL::STP::Action;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Coordinate;
using namespace AI::HL::Util;

namespace {

	class TriAttackPrime : public Tactic {
		public:
			TriAttackPrime(World world) : Tactic(world, true), attempted_shot(false) {
			}

		private:
			bool attempted_shot;
			bool done() const;
			bool fail() const;
			Player select(const std::set<Player> &players) const;
			void execute();
			Glib::ustring description() const {
				return "TriAttackPrime";
			}
	};

	Player TriAttackPrime::select(const std::set<Player> &players) const {
		Point position = AI::HL::STP::Evaluation::tri_attack_evaluation(world);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(position));
	}

	bool TriAttackPrime::done() const {
		// If the ball is shot or the player gets the ball or if the robot ends up in our side of the field.

		return attempted_shot;
	}

	bool TriAttackPrime::fail() const {
		return their_ball(world) || (ball_on_our_side(world) && !ball_midfield(world));
	}

	void TriAttackPrime::execute() {
		Point target_point = AI::HL::STP::Evaluation::tri_attack_evaluation(world);

		AI::HL::STP::Action::dribble(world, player, target_point);
	}

	class TriAttack2 : public Tactic {
		public:
			TriAttack2(World world) : Tactic(world) {
			}

		private:
			Player select(const std::set<Player> &players) const;
			void execute();
				Glib::ustring description() const {
				return "TriAttack2";
			}
	};

	Player TriAttack2::select(const std::set<Player> &players) const {
		Point position = AI::HL::STP::Evaluation::tri_attack_evaluation(world);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(position));
	}

	void TriAttack2::execute() {
		auto waypoints = Evaluation::evaluate_tri_attack();
		//Point destination = AI::HL::STP::Evaluation::tri_attack_evaluation(world);
		//destination = destination + Point(0.3, 0.3);
		Action::move(world, player, waypoints[1]);
	}

	class TriAttack3 : public Tactic {
		public:
			TriAttack3(World world) : Tactic(world) {
			}

		private:
			Player select(const std::set<Player> &players) const;
			void execute();
			Glib::ustring description() const {
				return "TriAttack3";
			}
	};

	Player TriAttack3::select(const std::set<Player> &players) const {
		Point position = AI::HL::STP::Evaluation::tri_attack_evaluation(world);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(position));
	}

	void TriAttack3::execute() {
		auto waypoints = Evaluation::evaluate_tri_attack();
		Point destination = AI::HL::STP::Evaluation::tri_attack_evaluation(world);
		destination = destination - Point(0.3, 0.3);
		Action::move(world, player, waypoints[2]);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::tri_attack_primary(AI::HL::W::World world) {
	Tactic::Ptr p(new TriAttackPrime(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tri_attack_secondary(AI::HL::W::World world) {
	Tactic::Ptr p(new TriAttack2(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::tri_attack_tertiary(AI::HL::W::World world) {
	Tactic::Ptr p(new TriAttack3(world));
	return p;
}
