#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/play/simple_play.h"

using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - Playtype Prepare Penalty Friendly
 *
 * Objective:
 * - move to Penalty positions and shoot the ball to enemy goal
 */
BEGIN_PLAY(PenaltyFriendly)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY)
	|| Predicates::playtype(world, AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY))
	&& Predicates::our_team_size_at_least(world, 2))
APPLICABLE(true)
DONE(Predicates::goal(world))
FAIL(false)
BEGIN_ASSIGN()

Point penalty_position1(0, 1.5);
Point penalty_position2(0, -1.5);
Point penalty_position3(0, -0.75);
Point penalty_position4(0, 0.75);

// GOALIE (LONE)
goalie_role.push_back(lone_goalie(world));

// ROLE 1
roles[0].push_back(wait_playtype(world, move(world,
	Point(world.field().penalty_enemy().x - 2 * Robot::MAX_RADIUS, 0)),
	AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY));
roles[0].push_back(penalty_shoot(world));

// ROLE 2
// move to penalty position 1
roles[1].push_back(move(world, penalty_position1));

// ROLE 3
// move to penalty position 2
roles[2].push_back(move(world, penalty_position2));

// ROLE 4
// move to penalty position 3
roles[3].push_back(move(world, penalty_position3));

// ROLE 5
// move to penalty position 4
roles[4].push_back(move(world, penalty_position4));

END_ASSIGN()
END_PLAY()
