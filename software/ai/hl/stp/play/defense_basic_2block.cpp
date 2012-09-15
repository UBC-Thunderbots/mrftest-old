#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/tdefend.h"

using AI::HL::STP::Enemy;

/**
 * Condition:
 * - When we are not on the offensive. 
 *
 * Objective:
 * - Gain control of the ball and block enemy players. 
 */
BEGIN_PLAY(DBasic2Block)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3))
APPLICABLE(defensive(world) && ball_x_less_than(world, -world.field().centre_circle_radius()))
DONE(offensive(world))
FAIL(!ball_x_less_than(world, -world.field().centre_circle_radius()) && !offensive(world))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// cm active def
roles[0].push_back(tactive_def(world));

// ROLE 2
// duo defender
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 & 4 (optional)
// block enemies closest to our goal
roles[2].push_back(block_goal(world, Enemy::closest_friendly_goal(world, 0)));
roles[3].push_back(block_goal(world, Enemy::closest_friendly_goal(world, 1)));

// ROLE 5 (optional)
// offend
roles[4].push_back(offend(world));

END_ASSIGN()
END_PLAY()

