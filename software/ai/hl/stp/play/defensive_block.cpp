#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	/**
	 * Condition:
	 * - ball in own end enemy has ball control
	 * - at least 2 players
	 *
	 * Objective:
	 * - defend the net
	 * - try to grab the ball
	 */
	class DefensiveBlock : public Play {
		public:
			DefensiveBlock(const World &world);
			~DefensiveBlock();

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<DefensiveBlock> factory_instance("Defensive Block");

	const PlayFactory &DefensiveBlock::factory() const {
		return factory_instance;
	}

	DefensiveBlock::DefensiveBlock(const World &world) : Play(world) {
	}

	DefensiveBlock::~DefensiveBlock() {
	}

	bool DefensiveBlock::invariant() const {
		return Predicates::playtype(world, PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3);
	}

	bool DefensiveBlock::applicable() const {
		return Predicates::their_ball(world);
	}

	bool DefensiveBlock::done() const {
		return Predicates::our_ball(world) || Predicates::ball_on_their_side(world);
	}

	bool DefensiveBlock::fail() const {
		return false;
	}

	void DefensiveBlock::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {

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
		// defend
		roles[2].push_back(defend_duo_extra(world));

		// ROLE 4 (optional)
		// offend
		roles[3].push_back(offend(world));
	}
}

