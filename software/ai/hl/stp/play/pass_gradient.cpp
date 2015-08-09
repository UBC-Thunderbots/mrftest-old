#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/pass_gradient.h"
#include "ai/hl/stp/play/simple_play.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Pass in front to whoever.
 */
BEGIN_PLAY(PassGradient)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY)
		&& Predicates::our_team_size_at_least(world, 3))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()

// STEP 1
goalie_role.push_back(goalie_dynamic(world, 1));
roles[0].push_back(passer_gradient(world));
roles[1].push_back(passee_gradient(world));
roles[2].push_back(passee_gradient_secondary(world));
roles[3].push_back(defend_duo_extra1(world));

/*
roles[1].push_back(passee_gradient_receive(world));
roles[2].push_back(passee_gradient_secondary(world));
roles[0].push_back(defend_duo_defender(world));
roles[3].push_back(defend_duo_extra1(world));
*/
//roles[4].push_back(follow_baller_gradient(world)); //change this to defend
//roles[5].push_back(get_open(world)); //make this tactic

// Step 2


goalie_role.push_back(goalie_dynamic(world, 1));
roles[0].push_back(passee_gradient_receive(world)); //make this tactic
roles[1].push_back(defend_duo_defender(world));
roles[2].push_back(defend_duo_extra1(world));
roles[3].push_back(passee_gradient(world));
roles[4].push_back(passee_gradient_secondary(world));




END_ASSIGN()
END_PLAY()
