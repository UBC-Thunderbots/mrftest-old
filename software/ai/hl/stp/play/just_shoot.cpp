#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offense.h"
#include "ai/hl/stp/tactic/lone_goalie.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
using AI::HL::STP::Evaluation::ConeDefense;
namespace Predicates = AI::HL::STP::Predicates;

namespace {

	/**
	 * Condition:
	 * - ball under team possesion
	 *
	 * Objective:
	 * - shoot the ball to enemy goal
	 */
	class JustShoot : public Play {
		public:
			JustShoot(AI::HL::W::World &world);
			~JustShoot();

		private:
			ConeDefense cone_defense;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr> (&roles)[4]);
			const PlayFactory& factory() const;
	};

	PlayFactoryImpl<JustShoot> factory_instance("Just Shoot");

	const PlayFactory& JustShoot::factory() const {
		return factory_instance;
	}

	JustShoot::JustShoot(World &world) : Play(world), cone_defense(*this, world) {
	}

	JustShoot::~JustShoot() {
	}

	bool JustShoot::applicable() const {
		return Predicates::playtype(world, PlayType::PLAY)
			&& Predicates::our_team_size_at_least(world, 3)
			&& Predicates::our_ball(world);
	}

	bool JustShoot::done() const {
		return Predicates::goal(world);
	}

	bool JustShoot::fail() const {
		return Predicates::their_ball(world);
	}

	void JustShoot::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr> (&roles)[4]) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		// GOALIE
		goalie_role.push_back(lone_goalie(world));

		// ROLE 1
		// shoot
		roles[0].push_back(shoot(world));

		// ROLE 2
		// offensive support
		roles[1].push_back(offense(world));

		// ROLE 3
		// block nearest enemy
		roles[2].push_back(block(world, Enemy::closest_friendly_goal(world, 0)));

		// ROLE 4
		// block 2nd nearest enemy
		roles[3].push_back(block(world, Enemy::closest_friendly_goal(world, 1)));
	}
}

