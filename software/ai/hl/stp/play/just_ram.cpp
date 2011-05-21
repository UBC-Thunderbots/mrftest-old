#include "ai/hl/stp/tactic/ram.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/play/simple_play.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - at least 3 players
 * - ball under enemy possesion
 * - ball on our side of the field
 * - enemy baller has clear shot to our goal!
 *
 * Objective:
 * - Defend and ram the ball away (grab if possible)
 */
BEGIN_PLAY(JustRam)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3) && Predicates::enemy_baller_can_shoot(world))
APPLICABLE(Predicates::their_ball(world) && Predicates::ball_on_our_side(world))
DONE(!Predicates::their_ball(world) || Predicates::ball_on_their_side(world))
FAIL(false)
BEGIN_ASSIGN()
	// GOALIE
	goalie_role.push_back(defend_duo_goalie(world));

	// ROLE 1
	// grab the ball if possible
	roles[0].push_back(chase(world));

	// ROLE 2
	// defend
	roles[1].push_back(defend_duo_defender(world));

	// ROLE 3 (optional)
	// ram ball
	roles[2].push_back(ram(world));

	// ROLE 4 (optional)
	// ram ball
	roles[3].push_back(ram(world));
END_ASSIGN()
END_PLAY()
