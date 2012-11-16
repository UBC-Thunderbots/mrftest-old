#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/intercept.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - Ball is in our corner and we can pass to the origin. 
 *
 * Objective:
 * - Shoot the ball to origin (indirect pass)
 */
Point target(0 ,0);

BEGIN_PLAY(OurCornerShoot)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY)
		&& Predicates::our_team_size_at_least(world, 4))
APPLICABLE(Predicates::our_ball(world)
	&& Predicates::ball_in_our_corner(world)
	&& Predicates::baller_can_pass_target(world, target))
DONE(false)
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// shoot towards the midfield
roles[0].push_back(shoot_target(world, target));

// ROLE 3
// move to where ball will be
roles[1].push_back(move(world, target));

// ROLE 2
// defend
roles[2].push_back(defend_duo_defender(world));

// ROLE 2
// defend
roles[3].push_back(defend_duo_extra1(world));

// ROLE 4 (optional)
// offend
roles[4].push_back(offend(world));

// SECOND STEP

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 0
// defend
roles[0].push_back(defend_duo_defender(world));

// Recieve Pass
roles[1].push_back(intercept(world));

// ROLE 4 (optional)
// block
roles[2].push_back(defend_duo_extra1(world));

// ROLE 3
// defend
roles[3].push_back(offend(world));

// ROLE 3
// defend
roles[4].push_back(offend_secondary(world));

END_ASSIGN()
END_PLAY()

