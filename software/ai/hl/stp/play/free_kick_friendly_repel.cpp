#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/repel.h"

namespace Predicates = AI::HL::STP::Predicates;

BEGIN_PLAY(FreeKickFriendlyRepel)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)) && Predicates::our_team_size_at_least(world, 2) && !Predicates::baller_can_shoot(world) && !Predicates::baller_can_pass(world))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// kicker
roles[0].push_back(repel(world));

// ROLE 2
// defend 1
roles[1].push_back(defend_duo_defender(world));

// ROLE 3
// defend 2
roles[2].push_back(defend_duo_extra(world));

// ROLE 4
// offend
roles[3].push_back(offend(world));
END_ASSIGN()
END_PLAY()

