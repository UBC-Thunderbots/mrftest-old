#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/tactic/repel.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/common/playtype.h"

namespace Predicates = AI::HL::STP::Predicates;

BEGIN_PLAY(FreeKickFriendlyRepel)
INVARIANT(
		(Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY)
		 || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY))
		&& Predicates::our_team_size_at_least(world, 2))

APPLICABLE(
		(Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY)
			|| Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)))

DONE(false)
FAIL(false)

BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// kicker
roles[0].push_back(corner_repel(world));

// ROLE 2
// defend 1
roles[1].push_back(defend_duo_defender(world));

// ROLE 3
// defend 2
roles[2].push_back(defend_duo_extra1(world));

// ROLE 4
// position to centre
roles[3].push_back(move(world, Point(0, 0)));

// ROLE 5
// offend
roles[3].push_back(offend(world));

END_ASSIGN()
END_PLAY()

