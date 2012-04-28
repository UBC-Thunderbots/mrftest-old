#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/penalty_goalie.h"
#include "ai/hl/stp/play/simple_play.h"

using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	const double PENALTY_MARK_LENGTH = 0.75;
	const double RESTRICTED_ZONE_LENGTH = 0.85;
}

/**
 * Condition:
 * - Playtype Prepare Penalty Enemy
 *
 * Objective:
 * - move to Penalty positions and shoot the ball to enemy goal
 */
BEGIN_PLAY(PenaltyEnemy)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::PREPARE_PENALTY_ENEMY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_PENALTY_ENEMY)) && Predicates::our_team_size_at_least(world, 1))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(penalty_goalie(world));

// ROLE 1
// move to penalty position 1
roles[0].push_back(move(world, Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 6 * Robot::MAX_RADIUS)));

// ROLE 2
// move to penalty position 2
roles[1].push_back(move(world, Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 3 * Robot::MAX_RADIUS)));

// ROLE 3
// move to penalty position 3
roles[2].push_back(move(world, Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -3 * Robot::MAX_RADIUS)));

// ROLE 4
// move to penalty position 4
roles[3].push_back(move(world, Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -6 * Robot::MAX_RADIUS)));

// ROLE 5
// move to penalty position 5
roles[4].push_back(move(world, Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + 4 * Robot::MAX_RADIUS, 0 * Robot::MAX_RADIUS)));

END_ASSIGN()
END_PLAY()

