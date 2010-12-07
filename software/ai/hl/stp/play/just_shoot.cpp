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
			std::vector<Tactic::Ptr> tick();
			double change_probability() const;
	};

	JustShootPlay::JustShootPlay(World &world) : Play(world) {
	}

	JustShootPlay::~JustShootPlay() {
	}

	double JustShootPlay::change_probability() const {
		return 1.0;
	}

	std::vector<Tactic::Ptr> JustShootPlay::tick() {
		std::vector<Tactic::Ptr> tactics(5);

		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
		std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().friendly_goal()));

		// GOALIE
		if (players.size() > 0 && players[0]->has_ball()) {
			tactics[0] = repel(world);
		} else {
			tactics[0] = defend_goal(world);
		}

		// ROLE 1
		// shoot the ball!
		tactics[1] = shoot(world);

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

	class JustShootPlayManager : public PlayManager {
		public:
			JustShootPlayManager() : PlayManager("Just Shoot") {
			}
			Play::Ptr create_play(World &world) const {
				const Play::Ptr p(new JustShootPlay(world));
				return p;
			}
			double score(World& world, bool) const;
	} factory_instance;

	///////////////////////////////////////////////////////////////////////////

	double JustShootPlayManager::score(World& world, bool) const {
		// check if we do not have ball
		FriendlyTeam& friendly = world.friendly_team();
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get(i)->has_ball()) return 0.5;
		}
		return 0;
	}

}

