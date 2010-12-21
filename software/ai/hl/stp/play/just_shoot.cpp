#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/basic.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {

	/**
	 * Condition:
	 * - ball under team possesion
	 *
	 * Objective:
	 * - shoot the ball to enemy goal
	 */
	class JustShootPlay : public Play {
		public:
			JustShootPlay(AI::HL::W::World &world);
			~JustShootPlay();
		private:
			bool done();
			void assign(std::vector<Tactic::Ptr>& goalie_role, std::vector<Tactic::Ptr>& role1, std::vector<Tactic::Ptr>& role2, std::vector<Tactic::Ptr>& role3, std::vector<Tactic::Ptr>& role4);
	};

	class JustShootPlayManager : public PlayManager {
		public:
			JustShootPlayManager() : PlayManager("Just Shoot") {
			}
			Play::Ptr create_play(World &world) const {
				const Play::Ptr p(new JustShootPlay(world));
				return p;
			}
			bool applicable(World &world) const;
	} factory_instance;

	bool JustShootPlayManager::applicable(World &world) const {
		// check if we do not have ball
		FriendlyTeam &friendly = world.friendly_team();
		if (friendly.size() < 2) return false;
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get(i)->has_ball()) {
				return true;
			}
		}
		return false;
	}

	JustShootPlay::JustShootPlay(World &world) : Play(world) {
	}

	JustShootPlay::~JustShootPlay() {
	}

	bool JustShootPlay::done() {
		return factory_instance.applicable(world);
	}

	void JustShootPlay::assign(std::vector<Tactic::Ptr>& goalie_role, std::vector<Tactic::Ptr>& role1, std::vector<Tactic::Ptr>& role2, std::vector<Tactic::Ptr>& role3, std::vector<Tactic::Ptr>& role4) {

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
		std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().friendly_goal()));

		// GOALIE
		if (players[0]->has_ball()) {
			goalie_role.push_back(repel(world));
		} else {
			goalie_role.push_back(defend_goal(world));
		}

		// ROLE 1
		// shoot
		if (players[0]->has_ball()) {
			if (enemies.size() > 0) {
				role1.push_back(block(world, enemies[0]));
			} else {
				role1.push_back(idle(world));
			}
		} else {
			role1.push_back(shoot(world));
		}

		// ROLE 2
		// block nearest enemy
		if (enemies.size() > 0) {
			role2.push_back(block(world, enemies[0]));
		} else {
			role2.push_back(idle(world));
		}

		// ROLE 3
		// block 2nd nearest enemy
		if (enemies.size() > 1) {
			role3.push_back(block(world, enemies[1]));
		} else {
			role3.push_back(idle(world));
		}

		// ROLE 4
		// block 3rd nearest enemy
		if (enemies.size() > 2) {
			role4.push_back(block(world, enemies[2]));
		} else {
			role4.push_back(idle(world));
		}	
	}
}

