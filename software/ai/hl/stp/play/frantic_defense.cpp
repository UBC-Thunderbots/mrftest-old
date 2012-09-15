#include "ai/hl/stp/tactic/repel.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/play/simple_play.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - When the ball is near our goal and enemy baller can shoot towards our goal. 
 *
 * Objective:
 * - Get the ball away from our goal at all cost!
 */
BEGIN_PLAY(FranticDefense)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) 
	&& Predicates::our_team_size_at_least(world, 3) 
	&& (Predicates::enemy_baller_can_shoot(world) || Predicates::ball_near_friendly_goal(world)))
APPLICABLE(true)
DONE(!Predicates::their_ball(world) && !ball_near_friendly_goal(world))
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// try to steal or repel ball away
roles[0].push_back(tactive_def(world));

// ROLE 2
// duo defender
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// defend extra
roles[2].push_back(defend_duo_extra1(world));

// ROLE 4 (optional)
// block
roles[3].push_back(defend_duo_extra2(world));

// ROLE 5 (optional)
// offend
roles[4].push_back(block_ball(world, Enemy::closest_ball(world, 1)));

END_ASSIGN()
END_PLAY()

