#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;


BEGIN_PLAY(RecieveTest)
INVARIANT(true)
APPLICABLE(false)
DONE(false)
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()


#warning very hacky here
Point add(0, 0);
if(world.ball().velocity().len() > 0.1){
	add = 1.5*world.ball().velocity().norm();
}
Point target = world.ball().position() + add;



// the only thing that matters here is the passee
// all other tactics are just placeholders

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// passer
// roles[0].push_back(passer_shoot_target(world, target));

// ROLE 2
// passee
roles[0].push_back(passee_move_target(world, target, true));


// ROLE 2
// passee
roles[1].push_back(defend_duo_defender(world));

// ROLE 3
// defend
roles[2].push_back(defend_duo_defender(world));

// ROLE 4
// offensive support through blocking closest enemy to ball
roles[3].push_back(block(world, Enemy::closest_ball(world, 0)));
END_ASSIGN()
END_PLAY()


