#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/lone_goalie.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Predicates;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;

namespace {

	/**
	 * Condition:
	 * - ball not under team possesion
	 * - have at least 2 players (one goalie, one stealer)
	 *
	 * Objective:
	 * - steal the ball from the enemy or just grab it if it's under nobody's possesion
	 */
	class StealOffensive : public Play {
		public:
			StealOffensive(AI::HL::W::World &world);
			~StealOffensive();

		private:
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles);
			const PlayFactory& factory() const;
	};

	PlayFactoryImpl<StealOffensive> factory_instance("Steal Offensive");

	const PlayFactory& StealOffensive::factory() const {
		return factory_instance;
	}

	StealOffensive::StealOffensive(World &world) : Play(world) {
	}

	StealOffensive::~StealOffensive() {
	}

	bool StealOffensive::applicable() const {
		return playtype(PlayType::PLAY)->evaluate(world)
			&& our_team_size_at_least(2)->evaluate(world)
			&& (their_ball()->evaluate(world) 
			|| none_ball()->evaluate(world));
	}

	bool StealOffensive::done() const {
		return our_ball()->evaluate(world);
	}

	bool StealOffensive::fail() const {
		// wait till timeout or done?
		return false;
	}

	void StealOffensive::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		//FriendlyTeam &friendly = world.friendly_team();

		// GOALIE
		goalie_role.push_back(lone_goalie(world));

		// TODO: add stealing tactics
		
		// ROLE 1
		// Stealer 
		roles[0].push_back(chase(world));
		//roles[0].push_back();

		// ROLE 2
		// Stealer2
		roles[1].push_back(chase(world));
		//roles[1].push_back();
		
		// ROLE 3
		// block nearest enemy
		roles[2].push_back(block(world, Enemy::closest_friendly_goal(world, 0)));

		// ROLE 4
		// block 2nd nearest enemy
		roles[3].push_back(block(world, Enemy::closest_friendly_goal(world, 1)));
	}
}

