#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/play/simple_play.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - Playtype Free Kick Enemy
 *
 * Objective:
 * - Handle Enemy Free Kick
 */
BEGIN_PLAY(FreeKickEnemy)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY)) && Predicates::our_team_size_at_least(world, 2))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(wait_playtype(world, defend_duo_goalie(world), AI::Common::PlayType::PLAY));

// ROLE 1
// defend
roles[0].push_back(defend_duo_defender(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_extra(world));

// ROLE 3
// offend
roles[2].push_back(offend(world));

// ROLE 4
// offend
roles[3].push_back(offend(world));
END_ASSIGN()
END_PLAY()

