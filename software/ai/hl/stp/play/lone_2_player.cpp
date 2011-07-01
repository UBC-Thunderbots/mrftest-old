#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend_solo.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - 2 players left
 *
 * Objective:
 * - lone goalie, shoot
 */
BEGIN_PLAY(Lone2Player)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_exactly(world, 2))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(lone_goalie(world));

// ROLE 1
// shoot
roles[0].push_back(shoot_goal(world));
END_ASSIGN()
END_PLAY()

