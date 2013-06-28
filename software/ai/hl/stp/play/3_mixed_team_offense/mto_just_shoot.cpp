#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"

using AI::HL::STP::Enemy;

/**
 * Condition:
 * - Playtype Play
 *
 * Objective:
 * - just shoot at the enemy goal
 */
BEGIN_PLAY(MTOJustShoot)
INVARIANT(playtype(world, PlayType::PLAY)
		&& our_team_size_at_least(world, 2))
APPLICABLE(our_ball(world)
	&& ball_x_greater_than(world, -0.4))
DONE(goal(world))
FAIL(their_ball(world))
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// shoot
roles[0].push_back(shoot_goal(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
roles[2].push_back(defend_duo_extra1(world));

// ROLE 4 (optional)
// offend
roles[3].push_back(offend(world));

// ROLE 5 (optional)
// offend
roles[4].push_back(offend_secondary(world));

END_ASSIGN()
END_PLAY()

