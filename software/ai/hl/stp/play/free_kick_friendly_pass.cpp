#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/pass.h"
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
	 * - Playtype Free Kick Friendly
	 * - Baller can't shoot at enemy ball but can pass to some other player
	 *
	 * Objective:
	 * - Handle Friendly Free Kick
	 */
	class FreeKickFriendlyPass : public Play {
		public:
			FreeKickFriendlyPass(const World &world);

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<FreeKickFriendlyPass> factory_instance("Free Kick Friendly Pass");

	const PlayFactory &FreeKickFriendlyPass::factory() const {
		return factory_instance;
	}

	FreeKickFriendlyPass::FreeKickFriendlyPass(const World &world) : Play(world) {
	}

	bool FreeKickFriendlyPass::invariant() const {
		return (Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)) && Predicates::our_team_size_at_least(world, 3) && !Predicates::baller_can_shoot(world) && Predicates::baller_can_pass(world);
	}

	bool FreeKickFriendlyPass::applicable() const {
		return true;
	}

	bool FreeKickFriendlyPass::done() const {
		return false;
	}

	bool FreeKickFriendlyPass::fail() const {
		return false;
	}

	void FreeKickFriendlyPass::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		
		// GOALIE
		goalie_role.push_back(defend_duo_goalie(world));

		// ROLE 1
		// passer
		roles[0].push_back(passer_shoot(world));
		roles[0].push_back(offend(world));

		// ROLE 2
		// passee
		roles[1].push_back(passee_move(world));

		// ROLE 3
		// defend
		roles[2].push_back(defend_duo_defender(world));

		// ROLE 4
		// offend
		roles[3].push_back(offend(world));
	}
}

