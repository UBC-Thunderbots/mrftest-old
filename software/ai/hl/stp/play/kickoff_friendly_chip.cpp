#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/free_kick_pass.h"

using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Coordinate;

namespace {
	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

	// distance for the offenders to be positioned away from the kicker
	const double SEPARATION_DIST = 10 * Robot::MAX_RADIUS;

	// hard coded positions for the kicker, and 2 offenders
	Point kicker_position = Point(-0.5 - Ball::RADIUS - Robot::MAX_RADIUS, 0);
	Point ready_positions[2] = { Point(-AVOIDANCE_DIST, -SEPARATION_DIST), Point(-AVOIDANCE_DIST, SEPARATION_DIST) };
}

/**
 * Condition:
 * - It is the execute friendly kickoff play
 *
 * Objective:
 * - Pass the ball to a friendly player without double touching the ball
 */
BEGIN_PLAY(KickoffFriendlyChip)
INVARIANT(Predicates::our_team_size_at_least(world, 2)
	&& (Predicates::playtype(world, AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY)
		|| Predicates::playtype(world, AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY)))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1 
// chip at half power
roles[0].push_back(wait_playtype(world, move(world, kicker_position), AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY));
//roles[0].push_back(free_kick_pass(world, world.field().enemy_goal(), true));
roles[0].push_back(free_kick_pass(world, world.field().enemy_goal(), true, 0.67));

// ROLE 2
roles[1].push_back(defend_duo_defender(world));

// ROLE 3
roles[2].push_back(move(world, ready_positions[0]));

// ROLE 4
roles[3].push_back(move(world, ready_positions[1]));

// ROLE 5
roles[4].push_back(defend_duo_extra1(world));

END_ASSIGN()
END_PLAY()

