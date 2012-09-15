#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/pass_ray.h"
#include "ai/hl/stp/tactic/pass_simple.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - Playtype Free Kick Friendly
 *
 * Objective:
 * - Handle Friendly Free Kick by passing by ray. 
 */
BEGIN_PLAY(FreeKickFriendlyRay)
INVARIANT(
		(Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY)
			|| Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY) 
			|| Predicates::playtype(world, AI::Common::PlayType::PLAY))
	&& Predicates::our_team_size_at_least(world, 3))
APPLICABLE((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY)
		|| Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)))

DONE(false)
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()

// STEP 1
goalie_role.push_back(goalie_dynamic(world, 1));
roles[0].push_back(passer_ray(world)); // ACTIVE
roles[1].push_back(defend_duo_defender(world));
roles[2].push_back(defend_duo_extra1(world));
roles[3].push_back(offend(world));
roles[4].push_back(offend_secondary(world));

END_ASSIGN()
END_PLAY()

