#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"

namespace Predicates = AI::HL::STP::Predicates;

namespace {

const Point target(1.2, -1.5);
BEGIN_PLAY(PassFixxed2)
INVARIANT(true)
APPLICABLE(Predicates::baller_can_shoot_target(world, target))
DONE(false)
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()

Point reciever_target(world.field().length()/2.0, 0);

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));
// ROLE 1
// passer
roles[0].push_back(passer_shoot_target(world, target));
// ROLE 2
// passee
roles[1].push_back(passee_move_target(world, target));
// ROLE 3
// defend
roles[2].push_back(defend_duo_defender(world));
// ROLE 4
// offensive support through blocking closest enemy to ball
roles[3].push_back(offend(world));

/////////////////////////////////////
// 2nd set of tactics 
/////////////////////////////////////

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));
// ROLE 1
// passer
roles[0].push_back(passee_receive_target(world, target));
// ROLE 2
// passee
roles[1].push_back(defend_duo_defender(world));
// ROLE 3
// defend
roles[2].push_back(offend(world));
// ROLE 4
// offensive support through blocking closest enemy to ball
roles[3].push_back(offend_secondary(world));

END_ASSIGN()
END_PLAY()

}
