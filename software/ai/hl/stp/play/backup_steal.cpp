#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/ball.h"

using AI::HL::STP::Enemy;

BEGIN_PLAY(BackUpSteal)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(fight_ball(world))
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// shoot
roles[0].push_back(back_up_steal(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// block
roles[2].push_back(block_ball(world, Enemy::closest_ball(world, 1)));

// ROLE 4 (optional)
// offensive support
roles[3].push_back(offend(world));

// ROLE 5 (optional)
// offensive support
roles[4].push_back(offend_secondary(world));
END_ASSIGN()
END_PLAY()

