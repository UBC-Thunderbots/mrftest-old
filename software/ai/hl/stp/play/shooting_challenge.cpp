#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/shooting_challenge.h"

using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	const double DIST_FROM_PENALTY_MARK = 0.4;
}

/**
 * Condition:
 * - Playtype Prepare Penalty Friendly
 *
 * Objective:
 * - move to Penalty positions and shoot the ball to enemy goal
 */
BEGIN_PLAY(PenaltyFriendly)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY)) && Predicates::our_team_size_at_least(world, 1))
APPLICABLE(true)
DONE(Predicates::goal(world))
FAIL(false)
BEGIN_ASSIGN()
//NO GOALIE ROLE!
// GOALIE (LONE)
//goalie_role.push_back(lone_goalie(world));

// ROLE 1
roles[0].push_back(move(world, Point(world.ball().position())));
roles[0].push_back(shooting_challenge(world, 2.0));

END_ASSIGN()
END_PLAY()
