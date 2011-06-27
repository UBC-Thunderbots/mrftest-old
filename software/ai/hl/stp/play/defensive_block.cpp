#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/cm_ball.h"
#include "ai/hl/stp/play/simple_play.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - ball in own end and enemy has ball control
 * - at least 3 players
 *
 * Objective:
 * - defend the net
 * - try to grab the ball
 */
BEGIN_PLAY(DefensiveBlock)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3) && !Predicates::enemy_baller_can_shoot(world) && Predicates::enemy_baller_can_pass(world))
APPLICABLE(Predicates::their_ball(world) && Predicates::ball_midfield(world) && Predicates::ball_on_our_side(world))
DONE(Predicates::our_ball(world) || Predicates::ball_on_their_side(world))
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
// defend the goal
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// defend
roles[0].push_back(defend_duo_defender(world));

// ROLE 2
// chase the ball!
roles[1].push_back(tactive_def(world));

// ROLE 3 (optional)
// defend
roles[2].push_back(defend_duo_extra(world));

// ROLE 4 (optional)
// offend
// roles[3].push_back(offend(world));
// block instead of offend (hence the name of the play?)
roles[3].push_back(block_ball(world, Enemy::closest_pass(world, Enemy::closest_ball(world, 0)->evaluate(), 0)));
END_ASSIGN()
END_PLAY()

