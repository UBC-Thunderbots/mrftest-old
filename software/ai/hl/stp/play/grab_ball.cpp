#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/lone_goalie.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Predicates;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;

namespace {

	const std::vector<const Predicate*> APPLICABLE = {
		playtype(PlayType::PLAY),
		our_team_size_at_least(3),
		none_ball(),
	};

	const std::vector<const Predicate*> DONE = {
		our_ball(),
	};

	const std::vector<const Predicate*> FAIL = {
		their_ball(),
	};

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
			GrabBall(World &world);
			~GrabBall();

		private:
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr> (&roles)[4]);
			const PlayFactory& factory() const;
	};

	PlayFactoryImpl<GrabBall> factory_instance("Grab Ball");

	const PlayFactory& GrabBall::factory() const {
		return factory_instance;
	}

	GrabBall::GrabBall(World &world) : Play(world) {
	}

	GrabBall::~GrabBall() {
	}

	bool GrabBall::applicable() const {
		return evaluate(world, APPLICABLE);
	}

	bool GrabBall::done() const {
		return evaluate(world, DONE);
	}

	bool GrabBall::fail() const {
		return evaluate(world, FAIL);
	}

	void GrabBall::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr> (&roles)[4]) {
		// GOALIE
		// defend the goal
		goalie_role.push_back(lone_goalie(world));

		// ROLE 1
		// chase the ball!
		roles[0].push_back(chase(world));

		// ROLE 2
		// block nearest enemy
		roles[1].push_back(block(world, Enemy::closest_friendly_goal(world, 0)));

		// ROLE 3
		// block 2nd nearest enemy
		roles[2].push_back(block(world, Enemy::closest_friendly_goal(world, 1)));

		// ROLE 4
		// block 3rd nearest enemy
		roles[3].push_back(block(world, Enemy::closest_friendly_goal(world, 2)));
	}
}

