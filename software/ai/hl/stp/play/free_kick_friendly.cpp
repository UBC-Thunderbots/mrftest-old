#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
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
	 * - Playtype Free Kick Friendly
	 *
	 * Objective:
	 * - Handle Friendly Free Kick
	 */
	class FreeKickFriendly : public Play {
		public:
			FreeKickFriendly(const World &world);
			~FreeKickFriendly();

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<FreeKickFriendly> factory_instance("Free Kick Friendly");

	const PlayFactory &FreeKickFriendly::factory() const {
		return factory_instance;
	}

	FreeKickFriendly::FreeKickFriendly(const World &world) : Play(world) {
	}

	FreeKickFriendly::~FreeKickFriendly() {
	}

	bool FreeKickFriendly::invariant() const {
		return (Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)) && Predicates::our_team_size_at_least(world, 3);
	}

	bool FreeKickFriendly::applicable() const {
		return true;
	}

	bool FreeKickFriendly::done() const {
#warning TODO
		return false;
	}

	bool FreeKickFriendly::fail() const {
#warning TODO
		return false;
	}

	void FreeKickFriendly::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.Friendly_team().get(0);
		// GOALIE
		goalie_role.push_back(defend_duo_goalie(world));

		// ROLE 1
		// kicker
		roles[0].push_back(shoot(world));
		
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

