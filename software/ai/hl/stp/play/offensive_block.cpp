#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
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
	 * - ball in their end and enemy has ball control
	 * - at least 3 players
	 *
	 * Objective:
	 * - defend the net
	 * - try to grab the ball and block passing by enemy baller
	 */
	class OffensiveBlock : public Play {
		public:
			OffensiveBlock(const World &world);

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<OffensiveBlock> factory_instance("Offensive Block");

	const PlayFactory &OffensiveBlock::factory() const {
		return factory_instance;
	}

	OffensiveBlock::OffensiveBlock(const World &world) : Play(world) {
	}

	bool OffensiveBlock::invariant() const {
		return Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3) && !Predicates::enemy_baller_can_shoot(world) && Predicates::enemy_baller_can_pass(world);
	}

	bool OffensiveBlock::applicable() const {
		return Predicates::their_ball(world) && Predicates::ball_midfield(world) && Predicates::ball_on_their_side(world);
	}

	bool OffensiveBlock::done() const {
		return !Predicates::their_ball(world);
	}

	bool OffensiveBlock::fail() const {
		return Predicates::ball_on_our_side(world); // should switch to defensive block
	}

	void OffensiveBlock::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {

		// GOALIE
		// defend the goal
		goalie_role.push_back(defend_duo_goalie(world));

		// ROLE 1
		// defend
		roles[0].push_back(defend_duo_defender(world));

		// ROLE 2
		// chase the ball!
		roles[1].push_back(chase(world));

		// ROLE 3 (optional)
		// offensive support through blocking possible passees of enemy baller
		roles[3].push_back(block_pass(world, Enemy::closest_pass(world, Enemy::closest_ball(world, 0)->evaluate(), 1)));

		// ROLE 4 (optional)
		// offensive support through blocking possible passees of enemy baller
		roles[3].push_back(block_pass(world, Enemy::closest_pass(world, Enemy::closest_ball(world, 0)->evaluate(), 2)));
	}
}

