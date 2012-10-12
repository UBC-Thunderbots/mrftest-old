#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/chip.h"

using AI::HL::STP::Coordinate;

/**
 * Condition:
 * - Playtype Play
 *
 * Objective:
 * - just chip at the enemy goal
 */
BEGIN_PLAY(JustChip)
INVARIANT(playtype(world, PlayType::PLAY)
		&& our_team_size_at_least(world, 2)
		&& baller_can_chip(world))
APPLICABLE(our_ball(world))
DONE(goal(world))
FAIL(their_ball(world))
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// chip
roles[0].push_back(chip_target(world, Coordinate(world, world.field().enemy_goal(), Coordinate::YType::BALL, Coordinate::OriginType::BALL)));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
roles[2].push_back(defend_duo_extra1(world));

// ROLE 4 (optional)
// offend
roles[3].push_back(offend(world));

// ROLE 5 (optional)
// offend
roles[4].push_back(offend_secondary(world));

END_ASSIGN()
END_PLAY()

