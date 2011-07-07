#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/shadow_kickoff.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - Playtype Free Kick Enemy
 *
 * Objective:
 * - Handle Enemy Free Kick
 */
BEGIN_PLAY(FreeKickEnemy)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY)))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(wait_playtype(world, goalie_dynamic(world, 0), AI::Common::PlayType::PLAY));

// ROLE 1
// defend
roles[0].push_back(defend_duo_defender(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_extra1(world));

// ROLE 3
roles[2].push_back(offend(world));

// ROLE 4
// move to other half of the field to position for cataching the ball after chipping shots
roles[3].push_back(shadow_ball(world));

END_ASSIGN()
END_PLAY()

