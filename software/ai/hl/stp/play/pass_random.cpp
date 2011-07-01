#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/play/simple_play.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Pass in front to whoever.
 */
BEGIN_PLAY(PassRandom)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY)
		&& Predicates::our_team_size_at_least(world, 3)
		&& Predicates::their_team_size_at_least(world, 1)
		&& !Predicates::baller_can_shoot(world)
		&& !Predicates::fight_ball(world))
APPLICABLE(Predicates::our_ball(world))
DONE(Predicates::baller_can_shoot(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()

// STEP 1
goalie_role.push_back(goalie_dynamic(world, 2));
roles[0].push_back(passer_random(world));
roles[1].push_back(offend(world));
roles[2].push_back(defend_duo_defender(world));
roles[3].push_back(offend_secondary(world));

// STEP 2
goalie_role.push_back(goalie_dynamic(world, 1));
roles[0].push_back(chase(world));
roles[1].push_back(defend_duo_defender(world));
roles[2].push_back(offend(world));
roles[3].push_back(offend_secondary(world));

END_ASSIGN()
END_PLAY()

