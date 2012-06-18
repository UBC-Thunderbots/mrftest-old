#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/pass_ray.h"
#include "ai/hl/stp/tactic/pass_simple.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Pass using ray.
 */
BEGIN_PLAY(PassRay)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY)
		&& Predicates::our_team_size_at_least(world, 3))
APPLICABLE(Predicates::our_ball(world)
		&& Predicates::can_shoot_ray(world))

DONE(false)
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()

// STEP 1
goalie_role.push_back(goalie_dynamic(world, 1));
roles[0].push_back(defend_duo_defender(world));
roles[1].push_back(defend_duo_extra1(world));
roles[2].push_back(passer_ray(world));
roles[3].push_back(follow_baller(world));
roles[4].push_back(passee_simple(world, 0));

END_ASSIGN()
END_PLAY()

