#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/chase.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Coordinate;

/**
 * Condition:
 * - Playtype Free Kick Friendly
 *
 * Objective:
 * - Handle Friendly Free Kick
 */
BEGIN_PLAY(FreeKickFriendly)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)) && Predicates::our_team_size_at_least(world, 3) && !Predicates::baller_can_pass(world))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// kicker
roles[0].push_back(chase(world));
roles[0].push_back(shoot_goal(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3
// offend
roles[2].push_back(offend(world));

// ROLE 4
// offend
roles[3].push_back(offend(world));
END_ASSIGN()
END_PLAY()

