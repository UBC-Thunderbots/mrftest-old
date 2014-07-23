#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/chip.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Coordinate;
using namespace AI::HL::STP::Play;
/**
 * Condition:
 * - Playtype Free Kick Friendly
 *
 * Objective:
 * - Handle Friendly Free Kick by simply shooting at the enemy goal. 
 */

namespace {
DoubleParam PARAM_free_kick_at_defence_zone(u8"Ball close to enemy defence area during free kick", u8"STP/freekick", 0.2, 0.0, 1.0);
BoolParam PARAM_free_kick_defence_enable(u8"Enable ball close to enemy defence area", u8"STP/freekick", true);
}

BEGIN_PLAY(FreeKickFriendly)
	INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)) && Predicates::our_team_size_at_least(world, 2))
	APPLICABLE(true)
	DONE(false)
	FAIL(false)
	BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// kicker
	if ((world.ball().position().x < 0) && baller_can_chip(world, true)) {
		//	roles[0].push_back(shoot_goal(world));
		//	roles[0].push_back(chip_target(world, world.field().enemy_corner_neg()));
			roles[0].push_back(chip_target(world, world.field().enemy_goal()));
	} else if (PARAM_free_kick_defence_enable && world.ball().position().x > world.field().width() / 2 - PARAM_free_kick_at_defence_zone && baller_can_chip(world, true)) {
		roles[0].push_back(chip_target(world, world.field().enemy_goal()- Point(world.field().defense_area_radius() / 2, 0)));
	} else {
//	roles[0].push_back(shoot_target(world, world.field().enemy_goal() - Point(-0.3, 0) ));
		roles[0].push_back(chip_target(world, world.field().enemy_goal()));
		}

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3
// offend
roles[2].push_back(offend(world));

// ROLE 4
// offend
roles[3].push_back(offend_secondary(world));

// ROLE 5
// extra defender
roles[4].push_back(defend_duo_extra1(world));
END_ASSIGN()
END_PLAY()

