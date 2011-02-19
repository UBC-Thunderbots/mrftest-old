#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/move_wait_playtype.h"
#include "ai/hl/stp/tactic/move.h"
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
	 * - Playtype Free Kick Enemy
	 *
	 * Objective:
	 * - Handle Enemy Free Kick
	 */
	class FreeKickEnemy : public Play {
		public:
			FreeKickEnemy(const World &world);
			~FreeKickEnemy();

		private:
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<FreeKickEnemy> factory_instance("Free Kick Enemy");

	const PlayFactory &FreeKickEnemy::factory() const {
		return factory_instance;
	}

	FreeKickEnemy::FreeKickEnemy(const World &world) : Play(world) {
	}

	FreeKickEnemy::~FreeKickEnemy() {
	}

	bool FreeKickEnemy::applicable() const {
		return (Predicates::playtype(world, PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) || Predicates::playtype(world, PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY)) && Predicates::our_team_size_at_least(world, 1);
	}

	bool FreeKickEnemy::done() const {
		return !(Predicates::playtype(world, PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) || Predicates::playtype(world, PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY)) ;
	}

	bool FreeKickEnemy::fail() const {
		return false;
	}

	void FreeKickEnemy::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.Enemy_team().get(0);
		// GOALIE
		goalie_role.push_back(defend_duo_goalie(world));

		// ROLE 1
		// defend
		roles[0].push_back(defend_duo_defender(world));
		
		// ROLE 2
		// defend
		roles[1].push_back(defend_duo_defender(world));
		
		// ROLE 3
		// offend
		roles[2].push_back(offend(world));
		
		// ROLE 4
		// offend
		roles[3].push_back(offend(world));
	}
}

