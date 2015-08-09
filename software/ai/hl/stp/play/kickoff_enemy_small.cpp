#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/free_kick_pass.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/play/simple_play.h"

using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	// the distance we want the players to the ball
	constexpr double AVOIDANCE_DIST = AI::HL::Util::KICKOFF_STOP_DIST +
		Ball::RADIUS + Robot::MAX_RADIUS;

	// distance for the offenders to be positioned away from the kicker
	constexpr double SEPARATION_DIST = 10 * Robot::MAX_RADIUS;

	// hard coded positions for the kicker, and 2 offenders
	constexpr Point kicker_position(-AVOIDANCE_DIST, 0);
	constexpr Point ready_positions[3] = { Point(0, -SEPARATION_DIST),
		Point(0, SEPARATION_DIST) };
}

/**
 * Condition:
 * - It is the execute friendly kickoff play
 *
 * Objective:
 * - Pass the ball to a friendly player without double touching the ball
 */
BEGIN_PLAY(KickOffEnemySmall)
INVARIANT(Predicates::our_team_size_at_least(world, 2) &&
	Predicates::their_team_size_at_most(world, 3) &&
	(Predicates::playtype(world, AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY)
	|| Predicates::playtype(world, AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY)))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
roles[0].push_back(wait_playtype(world, move(world, kicker_position),
	AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY));
roles[0].push_back(shoot_goal(world, true));

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


