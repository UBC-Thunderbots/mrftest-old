#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/repel.h"

using AI::HL::STP::Enemy;

BEGIN_PLAY(JustRepel)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3) && !baller_can_shoot(world) && !baller_can_pass(world))
APPLICABLE(our_ball(world) && ball_midfield(world))
DONE(goal(world))
FAIL(their_ball(world))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// repel
roles[0].push_back(repel(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// block
roles[2].push_back(block_ball(world, Enemy::closest_ball(world, 0)));

// ROLE 4 (optional)
// offensive support
roles[3].push_back(block_ball(world, Enemy::closest_ball(world, 1)));
END_ASSIGN()
END_PLAY()

