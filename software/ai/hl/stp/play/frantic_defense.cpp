#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/repel.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	/**
	 * Condition:
	 * - ball not under team possesion
	 * - ball not in corner or midfield
	 * - have at least 2 players (one goalie, one defender)
	 *
	 * Objective:
	 * - shoot the ball to enemy goal while passing the ball between the passer and passee
	 */
	class FranticDefense : public Play {
		public:
			FranticDefense(const AI::HL::W::World &world);

		private:
			bool invariant() const; 
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<FranticDefense> factory_instance("Frantic Defense");

	const PlayFactory &FranticDefense::factory() const {
		return factory_instance;
	}

	FranticDefense::FranticDefense(const World &world) : Play(world) {
	}

	bool FranticDefense::invariant() const {
		return Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 2);
	}

	bool FranticDefense::applicable() const {
		return !Predicates::our_ball(world) && !Predicates::ball_in_our_corner(world) && !Predicates::ball_in_their_corner(world) && !Predicates::ball_midfield(world) && Predicates::ball_on_our_side(world);
	}

	bool FranticDefense::done() const {
		return Predicates::our_ball(world) || Predicates::ball_in_our_corner(world) || Predicates::ball_in_their_corner(world) || Predicates::ball_midfield(world) || Predicates::ball_on_their_side(world);
	}

	bool FranticDefense::fail() const {
		return false;
	}

	void FranticDefense::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		// GOALIE
		goalie_role.push_back(defend_solo_goalie(world));
		
		// ROLE 1
		// try to grab ball
		roles[0].push_back(repel(world));

		// ROLE 2
		// defend extra
		roles[1].push_back(block(world, Enemy::closest_friendly_goal(world, 0)));

		// ROLE 3 (optional)
		// block 3st enemy
		roles[2].push_back(block(world, Enemy::closest_friendly_goal(world, 1)));

		// ROLE 4 (optional)
		// block 4nd enemy
		roles[3].push_back(block(world, Enemy::closest_friendly_goal(world, 2)));
	}
}

