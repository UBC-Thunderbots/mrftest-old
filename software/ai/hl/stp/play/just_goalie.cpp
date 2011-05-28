#include "ai/hl/stp/play/simple_play.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - 1 player
 *
 * Objective:
 * - have the goalie defend the goal
 */
BEGIN_PLAY(JustGoalie)
INVARIANT(Predicates::our_team_size_exactly(world, 1))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
goalie_role.push_back(active_solo_goalie(world));
END_ASSIGN()
END_PLAY()
