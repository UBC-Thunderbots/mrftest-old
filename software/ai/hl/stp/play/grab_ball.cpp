#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/basic.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	/**
	 * Condition:
	 * - ball not under any possesion
	 * - at least 2 players
	 *
	 * Objective:
	 * - grab the ball
	 */
	class GrabBallPlay : public Play {
		public:
			GrabBallPlay(AI::HL::W::World &world);
			~GrabBallPlay();

		private:
			bool done();
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr> &role1, std::vector<Tactic::Ptr> &role2, std::vector<Tactic::Ptr> &role3, std::vector<Tactic::Ptr> &role4);
	};

	class GrabBallPlayManager : public PlayManager {
		public:
			GrabBallPlayManager() : PlayManager("Grab Ball") {
			}
			Play::Ptr create_play(World &world) const {
				const Play::Ptr p(new GrabBallPlay(world));
				return p;
			}
			bool applicable(World &world) const;
	} factory_instance;

	bool GrabBallPlayManager::applicable(World &world) const {
		// check if we do not have ball
		FriendlyTeam &friendly = world.friendly_team();
		if (friendly.size() < 2) {
			return false;
		}
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get(i)->has_ball()) {
				return false;
			}
		}
		return true;
	}

	GrabBallPlay::GrabBallPlay(World &world) : Play(world) {
	}

	GrabBallPlay::~GrabBallPlay() {
	}

	bool GrabBallPlay::done() {
		return factory_instance.applicable(world);
	}

	void GrabBallPlay::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr> &role1, std::vector<Tactic::Ptr> &role2, std::vector<Tactic::Ptr> &role3, std::vector<Tactic::Ptr> &role4) {
		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
		std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().friendly_goal()));

		// GOALIE
		// defend the goal
		goalie_role.push_back(defend_goal(world));

		// ROLE 1
		// chase the ball!
		role1.push_back(chase(world));

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

