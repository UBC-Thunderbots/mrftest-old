#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/idle.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - 1 player
 *
 * Objective:
 * - have the goalie defend the goal
 */
BEGIN_PLAY(JustGoalie)
INVARIANT(Predicates::our_team_size_exactly(world, 1) && !Predicates::playtype(world, AI::Common::PlayType::PREPARE_PENALTY_ENEMY) && !Predicates::playtype(world, AI::Common::PlayType::EXECUTE_PENALTY_ENEMY)))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
goalie_role.push_back(lone_goalie_active(world));
roles[0].push_back(idle(world)); // just to get rid of warning
END_ASSIGN()
END_PLAY()

