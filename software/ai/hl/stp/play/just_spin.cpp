#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/cm_ball.h"

using AI::HL::STP::Enemy;

BEGIN_PLAY(JustSpin)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3) && !baller_can_shoot(world) && fight_ball(world))
APPLICABLE(true)
DONE(none_ball(world))
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// shoot
roles[0].push_back(tsteal(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// block
roles[2].push_back(block_ball(world, Enemy::closest_ball(world, 0)));

// ROLE 4 (optional)
// offensive support
roles[3].push_back(offend(world));
END_ASSIGN()
END_PLAY()

