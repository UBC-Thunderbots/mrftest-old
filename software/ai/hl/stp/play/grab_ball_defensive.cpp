#include "ai/hl/stp/tactic/intercept_v2.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/mark_offside.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Enemy;

/**
 * Condition:
 * - Ball not under team possession and the ball is in our side of the field
 *
 * Objective:
 * - Grab the ball, defensively
 */
BEGIN_PLAY(GrabBallDefensive)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY)
	&& Predicates::our_team_size_at_least(world, 2))
APPLICABLE(Predicates::none_ball(world)
	&& ball_x_less_than(world, -0.7))
DONE(Predicates::our_ball(world))
FAIL(Predicates::their_ball(world))

BEGIN_ASSIGN()
// GOALIE
// defend the goal
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// chase the ball!
roles[0].push_back(intercept_v2(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// defend extra
roles[2].push_back(defend_duo_extra1(world));

// ROLE 4 (optional)
// block 2
roles[3].push_back(shadow_enemy(world, 1));

// ROLE 5 (optional)
// offend
roles[4].push_back(defend_duo_extra2(world));

END_ASSIGN()
END_PLAY()

