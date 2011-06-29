#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/chase.h"

namespace Predicates = AI::HL::STP::Predicates;

BEGIN_PLAY(FreeKickFriendlyPass)
INVARIANT(
		(Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY)
		 || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)
		 || Predicates::playtype(world, AI::Common::PlayType::PLAY))
		&& Predicates::our_team_size_at_least(world, 3)
		&& !Predicates::baller_can_shoot(world))
APPLICABLE((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY)
			|| Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)))
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 2));

// ROLE 1
// passer
roles[0].push_back(passer_shoot_dynamic(world));
roles[0].push_back(passee_receive(world));

// ROLE 2
// passee
roles[1].push_back(passee_move_dynamic(world));
roles[1].push_back(offend(world));

// ROLE 3
// defend
roles[2].push_back(defend_duo_defender(world));

// ROLE 4
// offend
roles[3].push_back(offend(world));
roles[3].push_back(offend_secondary(world));
END_ASSIGN()
END_PLAY()

