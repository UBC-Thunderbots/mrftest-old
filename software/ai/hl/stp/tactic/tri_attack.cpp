#include "ai/hl/stp/tactic/tri_attack.h"
#include "ai/hl/stp/evaluation/tri_attack.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "ai/hl/stp/tactic/util.h"

using namespace AI::HL::STP::Tactic;
namespace Action = AI::HL::STP::Action;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Coordinate;

namespace {
/*	
	class tri_attack_primary : public Tactic {
		public:
			tri_attack_primary(World world) : Tactic(world, true), attempted_shot(false) {
			}

		private:
			bool attempted_shot;
			bool done() const;
			bool fail() const;
			Player select(const std::set<Player> &players) const;
			void execute();
			Glib::ustring description() const {
				return "tri_attack_primary";
			}
	};
*/
//	Player tri_attack::select(const std::set<Player> &players) const {
//		Point target = Evaluation::evaluate_t_attack(World world);
	//	return Player;/**std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(target)) */
	}
/*
	Player tri_attack_primary::select(const std::set<Player> &players) const
	{
		return select_baller(world, players, player);
	}

	bool tri_attack_primary::done() const {
		// If the ball is shot or the player gets the ball or if the robot ends up in our side of the field. 
		
		if player-> 
		return attempted_shot;
	}

	bool tri_attack_primary::fail() const {
		return their_ball(world) || (ball_on_our_side(world) && !ball_midfield(world));
	}

	void tri_attack_primary::execute() {
		Point target_point = AI::HL::STP::Evaluation::tri_offence_main(world);
		
		AI::HL::STP::Action::dribble(world, player, target_point); 					
	} */
//		Action::tri_attack_active(world, player, target);
	//}
//void tri_attack_active::execute(world, i) {
//do something
//}




	class tri_attack_secondaries : public Tactic {
		public:
			tri_attack_secondaries(World world, unsigned i) : Tactic(world), index(i) {
			}

		private:
			unsigned index;
			Player select(const std::set<Player> &players) const;
			void execute();
				Glib::ustring description() const {
				return "tri_attack_secondaries";
			}
	};

	Player tri_attack_secondaries::select(const std::set<Player> &players) const {
		Point position = AI::HL::STP::Evaluation::tri_attack_evaluation(world);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(position));
	}

	void tri_attack_secondaries::execute() {
		Point destination = AI::HL::STP::Evaluation::tri_attack_evaluation(world);
		destination = destination + Point(0.3, 0.3);
		Action::move(world, player, destination);
		}

	class tri_attack_tertiary : public Tactic {
		public:
			tri_attack_tertiary(World world, unsigned i) : Tactic(world), index(i) {
			}

		private:
			unsigned index;
			Player select(const std::set<Player> &players) const;
			void execute();
				Glib::ustring description() const {
				return "tri_attack_tertiary";
			}
	};

	Player tri_attack_tertiary::select(const std::set<Player> &players) const {
		Point position = AI::HL::STP::Evaluation::tri_attack_evaluation(world);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(position));
	}

	void tri_attack_tertiary::execute() {
		Point destination = AI::HL::STP::Evaluation::tri_attack_evaluation(world);
		destination = destination - Point(0.3, 0.3);
		Action::move(world, player, destination);
		}




