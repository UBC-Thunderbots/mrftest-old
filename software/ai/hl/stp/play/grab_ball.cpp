#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/block.h"
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
	class GrabBall : public Play {
		public:
			GrabBall(AI::HL::W::World &world);
			~GrabBall();

		private:
			void initialize();
			bool applicable() const;
			bool done();
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles);
			const PlayFactory& factory() const;
	};

	PlayFactoryImpl<GrabBall> factory_instance("Grab Ball");

	const PlayFactory& GrabBall::factory() const {
		return factory_instance;
	}

	void GrabBall::initialize() {
	}

	bool GrabBall::applicable() const {
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

	GrabBall::GrabBall(World &world) : Play(world) {
	}

	GrabBall::~GrabBall() {
	}

	bool GrabBall::done() {
		return applicable();
	}

	void GrabBall::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles) {
		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
		std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().friendly_goal()));

		// GOALIE
		// defend the goal
		goalie_role.push_back(defend_goal(world));

		// ROLE 1
		// chase the ball!
		roles[0].push_back(chase(world));

		// ROLE 2
		// block nearest enemy
		roles[1].push_back(block(world, AI::HL::STP::Evaluate::EnemyRole::closest_friendly_goal(world, 0)));

		// ROLE 3
		// block 2nd nearest enemy
		roles[2].push_back(block(world, AI::HL::STP::Evaluate::EnemyRole::closest_friendly_goal(world, 1)));

		// ROLE 4
		// block 3rd nearest enemy
		roles[3].push_back(block(world, AI::HL::STP::Evaluate::EnemyRole::closest_friendly_goal(world, 2)));
	}
}

