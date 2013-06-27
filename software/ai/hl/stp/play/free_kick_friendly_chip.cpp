#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/chip.h"
#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/tactic/move_wait_playtype.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Coordinate;

/**
 * Condition:
 * - Playtype Free Kick Friendly
 * - Baller can Chip
 *
 * Objective:
 * - Handle Friendly Free Kick by chipping to enemy goal. 
 */
BEGIN_PLAY(FreeKickFriendlyChip)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)) && Predicates::our_team_size_at_least(world, 2) && !Predicates::ball_in_our_corner(world) && !Predicates::ball_in_their_corner(world))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// kicker
roles[0].push_back(chip_target(world, world.field().enemy_goal()));
//roles[0].push_back(AI::HL::STP::Tactic::shoot_target(world, Point(world.field().enemy_goal())));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3
// offend
roles[2].push_back(defend_duo_extra1(world));

// ROLE 4
// offend
roles[3].push_back(offend(world));

// ROLE 5
// extra defender
roles[4].push_back(defend_duo_extra2(world));
END_ASSIGN()
END_PLAY()

