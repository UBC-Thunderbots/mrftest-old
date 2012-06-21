#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/tactic/mark_offside.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - Playtype Free Kick Enemy on our side
 *
 * Objective:
 * - Handle Enemy Free Kick on our side
 */
BEGIN_PLAY(FreeKickEnemyMarkDefend)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY)) && Predicates::ball_on_our_side(world))
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
// prevent one timer plays
roles[2].push_back(mark_offside(world));

// ROLE 4
// offend
roles[3].push_back(offend(world));

// ROLE 5
// defend
roles[4].push_back(defend_duo_extra2(world));

END_ASSIGN()
END_PLAY()

