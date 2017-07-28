#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/free_kick_to_goal.h"
#include "ai/hl/stp/tactic/defend_solo.h"

using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	// the distance we want the players to the ball
	constexpr double AVOIDANCE_DIST = Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

	// distance for the offenders to be positioned away from the kicker
	constexpr double SEPARATION_DIST = 10 * Robot::MAX_RADIUS;

	// hard coded positions for the kicker, and 2 offenders
	constexpr Point kicker_position(-0.5 - Ball::RADIUS - Robot::MAX_RADIUS, 0);
	constexpr Point ready_positions[2] = { Point(-AVOIDANCE_DIST, -SEPARATION_DIST), Point(-AVOIDANCE_DIST, SEPARATION_DIST) };
}

BEGIN_DEC(KickoffFriendly)
INVARIANT((playtype(world, PlayType::PREPARE_KICKOFF_FRIENDLY) || playtype(world, PlayType::EXECUTE_KICKOFF_FRIENDLY)) && our_team_size_at_least(world, 2))
APPLICABLE(true)
END_DEC(KickoffFriendly)

BEGIN_DEF(KickoffFriendly)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::lone_goalie(world);
tactics[1] = Tactic::move(world, kicker_position);
tactics[2] = Tactic::defend_duo_defender(world, true);
tactics[3] = Tactic::move(world, ready_positions[0]);
tactics[4] = Tactic::move(world, ready_positions[1]);
tactics[5] = Tactic::defend_duo_extra1(world, true);

while (!playtype(world, PlayType::EXECUTE_KICKOFF_FRIENDLY)) yield(caller);
tactics[1] = Tactic::free_kick_to_goal(world);

while (1) yield(caller);
END_DEF(KickoffFriendly)
