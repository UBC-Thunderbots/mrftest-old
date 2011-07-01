#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/play/simple_play.h"

using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	const double PENALTY_MARK_LENGTH = 0.45;
	const double RESTRICTED_ZONE_LENGTH = 0.85;
}

/**
 * Condition:
 * - Playtype Prepare Penalty Friendly
 *
 * Objective:
 * - move to Penalty positions and shoot the ball to enemy goal
 */
BEGIN_PLAY(PenaltyFriendly)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY)) && Predicates::our_team_size_at_least(world, 2))
APPLICABLE(true)
DONE(Predicates::goal(world))
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
#warning LONE GOALIE
goalie_role.push_back(lone_goalie(world));

// ROLE 1
// move to shooting position and shoot
roles[0].push_back(wait_playtype(world, move(world, Point(0.5 * world.field().length() - PENALTY_MARK_LENGTH - Robot::MAX_RADIUS, 0)), AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY));
roles[0].push_back(penalty_shoot(world));

// ROLE 2
// move to penalty position 1
roles[1].push_back(move(world, Point(0.5 * world.field().length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS)));

// ROLE 3
// move to penalty position 2
roles[2].push_back(move(world, Point(0.5 * world.field().length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS)));

// ROLE 4
// move to penalty position 3
roles[3].push_back(move(world, Point(0.5 * world.field().length() - RESTRICTED_ZONE_LENGTH - 5 * Robot::MAX_RADIUS, 0)));
END_ASSIGN()
END_PLAY()
