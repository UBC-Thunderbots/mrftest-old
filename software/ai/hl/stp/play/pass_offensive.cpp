#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

#include "ai/hl/stp/evaluation/offense.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

using namespace AI::HL::STP::Evaluation;

namespace {
	/**
	 * Condition:
	 * - ball under team possesion
	 * - have at least 4 players (one goalie, one passer, one passee, one defender)
	 *
	 * Objective:
	 * - shoot the ball to enemy goal while passing the ball between the passer and passee
	 */
	class PassOffensive : public Play {
		public:
			PassOffensive(const AI::HL::W::World &world);

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<PassOffensive> factory_instance("Pass Offensive");

	const PlayFactory &PassOffensive::factory() const {
		return factory_instance;
	}

	PassOffensive::PassOffensive(const World &world) : Play(world) {
	}

	bool PassOffensive::invariant() const {
		return Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3) && Predicates::their_team_size_at_least(world, 1) && !Predicates::baller_can_shoot(world);
	}

	bool PassOffensive::applicable() const {
		return Predicates::our_ball(world) && Predicates::ball_midfield(world);
	}

	bool PassOffensive::done() const {
		return Predicates::goal(world);
	}

	bool PassOffensive::fail() const {
		return Predicates::their_ball(world);
	}

	void PassOffensive::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		
		// GOALIE
		goalie_role.push_back(defend_duo_goalie(world));	
		
		// ROLE 1
		// passer
		roles[0].push_back(passer_shoot(world));
		roles[0].push_back(offend(world));

		// ROLE 2
		// passee
		roles[1].push_back(passee_receive(world));
		roles[1].push_back(shoot(world));

		// ROLE 3
		// defend
		roles[2].push_back(defend_duo_defender(world));

		// ROLE 4
		// offensive support through blocking closest enemy to ball
		roles[3].push_back(block(world, Enemy::closest_ball(world, 0)));
	}
}

