#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/cm_defense.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/cm_ball.h"
#include "ai/hl/stp/tactic/block.h"

using AI::HL::STP::Enemy;

BEGIN_PLAY(CMDBasic2Block)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(defensive(world) && ball_x_less_than(world,-0.7))
DONE(offensive(world))
FAIL(!ball_x_less_than(world,-0.7))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// cm active def
//roles[0].push_back(chase(world)); 
roles[0].push_back(tactive_def(world));

// ROLE 2
// block 1
roles[1].push_back(block(world, Enemy::closest_ball(world, 0)));

// ROLE 3 (optional)
// block 2
roles[2].push_back(block(world, Enemy::closest_ball(world, 1)));

// ROLE 4 (optional)
// duo defender
roles[3].push_back(defend_duo_extra(world));
END_ASSIGN()
END_PLAY()

