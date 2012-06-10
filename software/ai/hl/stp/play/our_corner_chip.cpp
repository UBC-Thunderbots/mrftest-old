#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/chip.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Enemy;
using AI::HL::STP::Coordinate;

/**
 * Condition:
 * - ball under team possession
 * - ball in one of our corners
 * - have at least 3 players (one goalie, one defender, one shooter)
 *
 * Objective:
 * - shoot the ball to enemy goal while passing the ball between the passer and passee
 */
BEGIN_PLAY(OurCornerChip)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY)
	&& Predicates::our_team_size_at_least(world, 3))
APPLICABLE(Predicates::our_ball(world)
		&& Predicates::ball_in_our_corner(world)
		&& !Predicates::baller_can_shoot(world)
		&& Predicates::baller_can_chip(world))
DONE(!Predicates::ball_in_our_corner(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// shoot towards the midfield
roles[0].push_back(chip_target(world, Point(0,0)));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// move to where the ball will be shot to
roles[2].push_back(move(world, Coordinate(world, Point(0,0), Coordinate::YType::ABSOLUTE, Coordinate::OriginType::ABSOLUTE)));

// ROLE 4 (optional)
// defend extra
roles[3].push_back(defend_duo_extra1(world));

// ROLE 4 (optional)
// offend
roles[4].push_back(offend(world));

END_ASSIGN()
END_PLAY()
