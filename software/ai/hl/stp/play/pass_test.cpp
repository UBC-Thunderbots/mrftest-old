#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"

namespace Predicates = AI::HL::STP::Predicates;

BEGIN_PLAY(PassDynamic)
INVARIANT(false && Predicates::playtype(world, AI::Common::PlayType::PLAY)
		&& Predicates::our_team_size_at_least(world, 3))
APPLICABLE(false
	&&	Predicates::our_ball(world))
DONE(false)
FAIL(Predicates::their_ball(world))

BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// passer
roles[0].push_back(passer_shoot_dynamic(world));

// ROLE 2
// passee
roles[1].push_back(passee_move_dynamic(world));

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
roles[0].push_back(passee_receive(world));

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

