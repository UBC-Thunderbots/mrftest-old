#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Evaluate::EnemyRole;

namespace {

	/**
	 * Condition:
	 * - ball under team possesion
	 * - have at least 3 players (one goalie, one passer, one passee)
	 *
	 * Objective:
	 * - shoot the ball to enemy goal while passing the ball between the passer and passee
	 */
	class PassOffensive : public Play {
		public:
			PassOffensive(AI::HL::W::World &world);
			~PassOffensive();

		private:
			void initialize();
			bool applicable() const;
			bool done();
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles);
			const PlayFactory& factory() const;
	};

	PlayFactoryImpl<PassOffensive> factory_instance("Pass Offensive");

	const PlayFactory& PassOffensive::factory() const {
		return factory_instance;
	}

	void PassOffensive::initialize() {
	}

	bool PassOffensive::applicable() const {
		// check if we do not have ball
		FriendlyTeam &friendly = world.friendly_team();
		if (friendly.size() < 4) {
			return false;
		}
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get(i)->has_ball()) {
				return true;
			}
		}
		return false;
	}

	PassOffensive::PassOffensive(World &world) : Play(world) {
	}

	PassOffensive::~PassOffensive() {
	}

	bool PassOffensive::done() {
		return applicable();
	}

	void PassOffensive::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		FriendlyTeam &friendly = world.friendly_team();

		// GOALIE
		goalie_role.push_back(defend_goal(world));

		// TODO: better passer and passee positioning and targeting
		
		// ROLE 1
		// passer 
		roles[0].push_back(passer_ready(world, friendly.get(1)->position(), friendly.get(2)->position()));

		// ROLE 2
		// passee
		roles[1].push_back(passee_ready(world, world.ball().position()));
		roles[1].push_back(shoot(world));

		// ROLE 3
		// block nearest enemy
		roles[2].push_back(block(world, EnemyRole::closest_friendly_goal(world, 0)));

		// ROLE 4
		// block 2nd nearest enemy
		roles[3].push_back(block(world, EnemyRole::closest_friendly_goal(world, 1)));
	}
}

