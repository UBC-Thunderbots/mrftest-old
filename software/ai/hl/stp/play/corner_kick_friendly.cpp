#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/free_kick_pass.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Coordinate;

/**
 *Chips ball to right infront of penalty spot from enemy corner
 *
 */
BEGIN_PLAY(CornerKickFriendly)
INVARIANT((Predicates::playtype(world, AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)) && Predicates::our_team_size_at_least(world, 2) && !Predicates::baller_can_shoot(world) &&
Predicates::baller_can_chip(world) && Predicates::ball_in_their_corner(world))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// kicker
roles[0].push_back(free_kick_pass(world, world.field().penalty_friendly(), true));

// ROLE 2
// defend
roles[1].push_back(offend(world));

// ROLE 3
// offend
roles[2].push_back(defend_duo_defender(world));

// ROLE 4
// offend
roles[3].push_back(offend_secondary(world));

// ROLE 5
// extra defender
roles[4].push_back(defend_duo_extra1(world));
END_ASSIGN()
END_PLAY()





