#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Pass, defenders have higher priority than passers
 */
BEGIN_PLAY(TriDefense2015)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY)
	&& Predicates::our_team_size_at_least(world, 4)
	&& Predicates::their_team_size_at_least(world, 3))
APPLICABLE(Predicates::their_ball(world)
	&& (Predicates::ball_midfield(world)
	|| Predicates::ball_on_our_side(world)))
DONE(Predicates::our_ball(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()

// STEP 1
goalie_role.push_back(defend_duo_goalie(world));
roles[0].push_back(defend_duo_defender(world));
roles[1].push_back(defend_duo_extra1(world));
roles[2].push_back(spin_steal(world));
roles[3].push_back(shadow_enemy(world, 0));
roles[4].push_back(shadow_enemy(world, 1));

END_ASSIGN()
END_PLAY()

