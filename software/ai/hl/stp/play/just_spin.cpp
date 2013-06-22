#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/ball.h"

using AI::HL::STP::Enemy;

/**
 * Condition:
 * - Playtype Play
 *
 * Objective:
 * - just fight for the ball
 */
BEGIN_PLAY(JustSpin)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2) && fight_ball(world))
APPLICABLE(true)
DONE(none_ball(world))
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// shoot
roles[0].push_back(spin_steal(world));

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
roles[4].push_back(defend_duo_extra1(world));
END_ASSIGN()
END_PLAY()

