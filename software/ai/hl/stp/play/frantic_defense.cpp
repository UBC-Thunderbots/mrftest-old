#include "ai/hl/stp/tactic/repel.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/cm_ball.h"
#include "ai/hl/stp/play/simple_play.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - ball not under team possesion
 * - ball not in corner or midfield
 * - ball on our side (hence very very close to our goal!)
 * - have at least 2 players (one goalie, one defender)
 *
 * Objective:
 * - get the ball away from our goal at all cost!
 */
BEGIN_PLAY(FranticDefense)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 2) && Predicates::their_ball(world))
APPLICABLE(!Predicates::ball_in_our_corner(world) && !Predicates::ball_in_their_corner(world) && !Predicates::ball_midfield(world) && Predicates::ball_on_our_side(world))
DONE(!Predicates::their_ball(world) || Predicates::ball_in_our_corner(world) || Predicates::ball_in_their_corner(world) || Predicates::ball_midfield(world) || Predicates::ball_on_their_side(world))
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(lone_goalie(world));

// ROLE 1
// try to repel ball away
//roles[0].push_back(repel(world));
roles[0].push_back(tactive_def(world));

// ROLE 2
// block the enemy baller
roles[1].push_back(block_goal(world, Enemy::closest_ball(world, 0)));

// ROLE 3 (optional)
// block enemy closest to our goal
roles[2].push_back(block_goal(world, Enemy::closest_friendly_goal(world, 0)));

// ROLE 4 (optional)
// block enemy closest to our goal
roles[3].push_back(block_goal(world, Enemy::closest_friendly_goal(world, 1)));
END_ASSIGN()
END_PLAY()

