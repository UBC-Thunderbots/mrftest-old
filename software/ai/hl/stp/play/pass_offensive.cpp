#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/pass.h"

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
			~PassOffensive();

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

	PassOffensive::~PassOffensive() {
	}

	bool PassOffensive::invariant() const {
		return Predicates::playtype(world, PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3);
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
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		const FriendlyTeam &friendly = world.friendly_team();
		
		std::set<Player::CPtr> players;
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			players.insert(friendly.get(i));
		}
		
		// GOALIE
		goalie_role.push_back(defend_duo_goalie(world));

#warning BROKEN, need proper passer and passee positioning and targeting
		// Temporary hack, should use proper pass evaluation
		std::pair <Point, Point> pp = calc_pass_positions(world, players);		
		
		// ROLE 1
		// passer
		roles[0].push_back(passer_ready(world, pp.first, pp.second));
		roles[0].push_back(passer_shoot(world, pp.second));
		roles[0].push_back(offend(world));

		// ROLE 2
		// passee
		roles[1].push_back(passee_ready(world, pp.second));
		roles[1].push_back(passee_receive(world, pp.second));
		roles[1].push_back(shoot(world));

		// ROLE 3
		// defend
		roles[2].push_back(defend_duo_defender(world));

		// ROLE 4
		// offensive support
		roles[3].push_back(offend(world));
	}
}

