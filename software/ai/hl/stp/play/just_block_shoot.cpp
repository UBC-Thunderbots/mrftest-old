#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"

using AI::HL::STP::Enemy;

BEGIN_PLAY(JustBlockShoot)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(our_ball(world) && !num_of_enemies_on_our_side_at_least(world,1))
DONE(goal(world))
FAIL(their_ball(world))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_solo_goalie(world));

// ROLE 1
// shoot
roles[0].push_back(shoot(world));

// ROLE 2
// block 1
roles[1].push_back(block_pass(world, Enemy::closest_ball(world, 0)));

// ROLE 3 (optional)
// block 2
roles[2].push_back(block_pass(world, Enemy::closest_ball(world, 1)));

// ROLE 4 (optional)
// block 3
roles[3].push_back(block_pass(world, Enemy::closest_ball(world, 2)));
END_ASSIGN()
END_PLAY()

