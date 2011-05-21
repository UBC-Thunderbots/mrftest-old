#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/play/simple_play.h"

using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const double AVOIDANCE_ANGLE = 2.0 * std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST);

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
BEGIN_PLAY(KickoffFriendly)
INVARIANT(Predicates::our_team_size_at_least(world, 2) && (Predicates::playtype(world, AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY)))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
	goalie_role.push_back(defend_duo_goalie(world));

	roles[0].push_back(wait_playtype(world, move(world, kicker_position), AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY));
	roles[0].push_back(shoot(world));

	roles[1].push_back(move(world, ready_positions[0]));

	roles[2].push_back(move(world, ready_positions[1]));

	roles[3].push_back(defend_duo_defender(world));
END_ASSIGN()
END_PLAY()
