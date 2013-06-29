#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/direct_free_friendly_pivot.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/move_active.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Coordinate;


/**
 * Condition:
 * - Playtype Free Kick Friendly
 *
 * Objective:
 * - Handle Friendly Free Kick by simply shooting at the enemy goal.
 */
BEGIN_PLAY(DirectKickFriendlyPivot)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) && Predicates::our_team_size_at_least(world, 2))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()

Point way_point(3.1, 2.3);
Point intermediate (2.9, 1.9);
Point start_point(1.5, 2.3);

if (world.ball().position().y < 0){
	start_point.y *= -1;
	way_point.y *= -1;
	intermediate.y *= -1;
}
if (world.ball().position().x < 0)  {
	start_point.x *= -1;
	way_point.x *= -1;
	intermediate.x *=-1;
}
#warning this offset needs to be tweaked. Consider adding a param.
if (world.ball().position().x > 0) start_point.x -= 0.3;
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// kicker
roles[0].push_back(move_active(world, start_point, (world.ball().position() - start_point).orientation(), false, true));
//roles[0].push_back(move_active(world, start_point, (world.ball().position() - start_point).orientation(), false));
//roles[0].push_back(move_active(world, intermediate, (world.ball().position() - intermediate).orientation(), false));
roles[0].push_back(move_active(world, way_point, (world.ball().position() - way_point).orientation(), false, true));
//roles[0].push_back(move_active(world, intermediate, (world.ball().position() - way_point).orientation(), false, true));
roles[0].push_back(direct_free_friendly_pivot(world));

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

