#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/move.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Enemy;
using AI::HL::STP::Coordinate;

/**
 * Condition:
 * - ball under team possesion
 * - ball in one of our corners
 * - have at least 3 players (one goalie, one defender, one shooter)
 *
 * Objective:
 * - shoot the ball to enemy goal while passing the ball between the passer and passee
 */
BEGIN_PLAY(OurCornerPlay)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3))
APPLICABLE(Predicates::our_ball(world) && Predicates::ball_in_our_corner(world) && !Predicates::baller_can_shoot(world) && !Predicates::baller_can_pass(world))
DONE(!Predicates::ball_in_our_corner(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// shoot towards the midfield
roles[0].push_back(shoot(world, Point(-world.ball().position().x, -world.ball().position().y)));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// block
roles[2].push_back(block_pass(world, Enemy::closest_ball(world, 0)));

// ROLE 4 (optional)
// move to where the ball will be shot to
roles[3].push_back(move(world, Coordinate(world, Point(-world.ball().position().x, -world.ball().position().y), Coordinate::YType::ABSOLUTE, Coordinate::OriginType::ABSOLUTE)));
END_ASSIGN()
END_PLAY()

