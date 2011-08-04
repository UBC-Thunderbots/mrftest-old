#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/play/simple_play.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = 0.50 + Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const Angle AVOIDANCE_ANGLE = 2.0 * Angle::of_radians(std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST));

	DegreeParam separation_angle("kickoff: angle to separate players (degrees)", "STP/Kickoff", 20, 0, 80);
}

/**
 * Condition:
 * - Playtype Kickoff Enemy
 *
 * Objective:
 * - Handle Enemy Kickoff
 */
BEGIN_PLAY(KickoffEnemy)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::PREPARE_KICKOFF_ENEMY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY)) && Predicates::our_team_size_at_least(world, 2))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(wait_playtype(world, defend_duo_goalie(world), AI::Common::PlayType::PLAY));

// ROLE 1
// defend
roles[0].push_back(defend_duo_defender(world));

// calculate angle between robots
const Angle delta_angle = AVOIDANCE_ANGLE + separation_angle;

// a ray that shoots from the center to friendly goal.
const Point shoot = Point(-1, 0) * AVOIDANCE_DIST;

// ROLE 2
// move to offender position 1
roles[1].push_back(move(world, shoot));

// ROLE 3
// move to offender position 2
roles[2].push_back(shadow_kickoff(world, Enemy::closest_ball(world, 1), shoot.rotate(delta_angle)));

// ROLE 4
// move to offender position 3
roles[3].push_back(shadow_kickoff(world, Enemy::closest_ball(world, 2), shoot.rotate(-delta_angle)));

END_ASSIGN()
END_PLAY()

