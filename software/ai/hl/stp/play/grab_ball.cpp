#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/basic.h"
#include "ai/hl/util.h"
#include "util/dprint.h"

#include <glibmm.h>

using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {

	/**
	 * Condition:
	 * - ball not under any possesion
	 *
	 * Objective:
	 * - grab the ball
	 */
	class GrabBallPlay : public Play {
		public:
			GrabBallPlay(AI::HL::W::World &world);
			~GrabBallPlay();
		private:
			std::vector<Tactic::Ptr> tick();
			double change_probability() const;
	};

	GrabBallPlay::GrabBallPlay(World &world) : Play(world) {
	}

	GrabBallPlay::~GrabBallPlay() {
	}

	double GrabBallPlay::change_probability() const {
		return 1.0;
	}

	std::vector<Tactic::Ptr> GrabBallPlay::tick() {
		std::vector<Tactic::Ptr> tactics(5);

		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
		std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().friendly_goal()));

		// GOALIE
		if (world.friendly_team().size() == 1) {
			tactics[0] = chase(world);
		} else {
			tactics[0] = defend_goal(world);
		}

		// ROLE 1
		// chase the ball!
		tactics[1] = chase(world);

		// ROLE 2
		// block nearest enemy
		if (enemies.size() > 0) {
			tactics[2] = block(world, enemies[0]);
		} else {
			tactics[2] = idle(world);
		}

		// ROLE 3
		// block 2nd nearest enemy
		if (enemies.size() > 1) {
			tactics[3] = block(world, enemies[1]);
		} else {
			tactics[3] = idle(world);
		}

		// ROLE 4
		// block 3rd nearest enemy
		if (enemies.size() > 2) {
			tactics[4] = block(world, enemies[2]);
		} else {
			tactics[4] = idle(world);
		}

		return tactics;
	}

	///////////////////////////////////////////////////////////////////////////
	// housekeeping code

	class GrabBallPlayManager : public PlayManager {
		public:
			GrabBallPlayManager() : PlayManager("Grab Ball") {
			}
			Play::Ptr create_play(World &world) const {
				const Play::Ptr p(new GrabBallPlay(world));
				return p;
			}
			double score(World& world, bool) const;
	} factory_instance;

	///////////////////////////////////////////////////////////////////////////

	double GrabBallPlayManager::score(World& world, bool) const {
		// check if we do not have ball
		FriendlyTeam& friendly = world.friendly_team();
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get(i)->has_ball()) return 0;
		}
		return 0.5;
	}

}

