#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Enemy;

/**
 * Condition:
 * - ball under team possession
 * - ball in their corner
 * - have at least 3 players (one goalie, one defender, one shooter)
 *
 * Objective:
 * - get the ball to a better shooting position (in the mid field)
 */
BEGIN_PLAY(TheirCornerPlay)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY)
		&& Predicates::our_team_size_at_least(world, 3))
APPLICABLE(Predicates::our_ball(world)
		&& Predicates::ball_in_their_corner(world)
		&& !Predicates::baller_can_shoot(world)
		&& !Predicates::baller_can_pass(world))
DONE(!Predicates::ball_in_their_corner(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// shoot towards the midfield (diagonally to our corner)
roles[0].push_back(shoot_target(world, Point(-world.ball().position().x, -world.ball().position().y)));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// block
roles[2].push_back(block_ball(world, Enemy::closest_ball(world, 0)));

// ROLE 4 (optional)
// offensive support
roles[3].push_back(offend(world));

// ROLE 5 (optional)
// defensive support
roles[4].push_back(defend_duo_extra1(world));

END_ASSIGN()
END_PLAY()
