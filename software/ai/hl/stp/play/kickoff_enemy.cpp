#include "../tactic/defend.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/enemy.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/shoot.h"

namespace {
	// the distance we want the players to the ball
	constexpr double AVOIDANCE_DIST = AI::HL::Util::KICKOFF_STOP_DIST + Ball::RADIUS
		+ Robot::MAX_RADIUS;

	// distance for the offenders to be positioned away from the kicker
	constexpr double SEPARATION_DIST = 10 * Robot::MAX_RADIUS;

	// hard coded positions for the kicker, and 2 offenders
	constexpr Point kicker_position(-AVOIDANCE_DIST, 0);
	constexpr Point ready_positions[3] = { Point(0, -SEPARATION_DIST),
		Point(0, SEPARATION_DIST) };
}

BEGIN_DEC(KickoffEnemy)
INVARIANT((playtype(world, PlayType::PREPARE_KICKOFF_ENEMY) || playtype(world, PlayType::EXECUTE_KICKOFF_ENEMY)) && our_team_size_at_least(world, 2))
APPLICABLE(true)
END_DEC(KickoffEnemy)

BEGIN_DEF(KickoffEnemy)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::defend_duo_goalie(world);
tactics[1] = Tactic::defend_duo_defender(world);
tactics[2] = Tactic::move(world, kicker_position);
tactics[3] = Tactic::shadow_kickoff(world, Enemy::closest_ball(world, 1), ready_positions[0]);
tactics[4] = Tactic::shadow_kickoff(world, Enemy::closest_ball(world, 2), ready_positions[1]);
tactics[5] = Tactic::move(world, Point(world.field().friendly_goal().x + world.field().defense_area_radius() + 3*Robot::MAX_RADIUS, 0));

while (1) yield(caller);
END_DEF(KickoffEnemy)
