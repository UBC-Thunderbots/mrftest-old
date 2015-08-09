#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/util.h"
#include "geom/param.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

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

/**
 * Condition:
 * - Playtype Kickoff Enemy
 *
 * Objective:
 * - Handle Enemy Kickoff
 */
BEGIN_PLAY(KickoffEnemy)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::PREPARE_KICKOFF_ENEMY)
	|| Predicates::playtype(world, AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY))
	&& Predicates::our_team_size_at_least(world, 2))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(wait_playtype(world, defend_duo_goalie(world), AI::Common::PlayType::PLAY));

// ROLE 1
// defend
roles[0].push_back(defend_duo_defender(world));

// ROLE 2
// move to offender position 1
roles[1].push_back(move(world, kicker_position));

// ROLE 3
// shadowing
roles[2].push_back(shadow_kickoff(world, Enemy::closest_ball(world, 1), ready_positions[0]));

// ROLE 4
// shadowing
roles[3].push_back(shadow_kickoff(world, Enemy::closest_ball(world, 2), ready_positions[1]));

// ROLE 5
// defend
roles[4].push_back(move(world, Point(world.field().friendly_goal().x + world.field().defense_area_radius() + 3*Robot::MAX_RADIUS, 0)));

END_ASSIGN()
END_PLAY()

