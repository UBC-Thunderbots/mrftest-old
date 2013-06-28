#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/direct_free_friendly_pivot.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/move_active.h"
#include "ai/hl/stp/tactic/indirect_chip_to_cherry.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Coordinate;


/**
 * Condition:
 * - Playtype In Direct Friendly
 *
 * Objective:
 * - Handle Friendly Free Kick by simply shooting at the enemy goal.
 */
BEGIN_PLAY(IndirectKickFriendlyPivot)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY) && Predicates::our_team_size_at_least(world, 2))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// kicker
roles[0].push_back(indirect_chip_to_cherry(world));
roles[0].push_back(direct_free_friendly_pivot(world));

// ROLE 2
// defend
roles[1].push_back(offend(world));

// ROLE 3
// offend
roles[2].push_back(defend_duo_defender(world));

// ROLE 4
// offend
roles[3].push_back(offend_secondary(world));

// ROLE 5
// extra defender
roles[4].push_back(defend_duo_extra1(world));
END_ASSIGN()
END_PLAY()

