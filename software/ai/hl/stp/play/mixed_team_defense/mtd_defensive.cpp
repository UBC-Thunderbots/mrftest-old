#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/play/simple_play.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Mixed team defense Defensive
 * four players
 */
BEGIN_PLAY(MTDDefensive)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3) && !Predicates::enemy_baller_can_shoot(world) && Predicates::enemy_baller_can_pass(world))
APPLICABLE(Predicates::their_ball(world) && (Predicates::ball_midfield(world) || Predicates::ball_on_our_side(world)))
DONE(!Predicates::their_ball(world) || Predicates::ball_on_their_side(world))
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
// defend the goal
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// active def
roles[0].push_back(tactive_def(world));

// ROLE 2
// defend duo
roles[1].push_back(defend_duo_defender(world));

// ROLE 3
// defend extra
roles[2].push_back(defend_duo_extra1(world));

END_ASSIGN()
END_PLAY()

