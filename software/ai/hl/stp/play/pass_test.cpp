#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"

namespace Predicates = AI::HL::STP::Predicates;

namespace {

	const Point targets[] = {
		Point(-1.2, 0),
		Point(-1, 0),
		Point(-1.2, 0.3),
		Point(-1.2, -0.3),
		Point(-1.2, 0),
		Point(-1.2, -0.3),
		Point(-1.2, 0),
		Point(-0.5, 0),
		Point(-1.0, 0),
		Point(-0.5, 1.2),
		Point(-1, -0.6),
		Point(-2, 0.6),
		Point(-1, -0.6),
		Point(-0.5, 0),
		Point(-1.3, 0.6),
		Point(-1.2, 0),
		Point(-1, 0),
		Point(-1.2, 0.3),
		Point(-1.2, -0.3),
		Point(-1.2, 0),
		Point(-1.2, -0.3),
		Point(-1.2, 0),
		Point(-0.5, 0),
		Point(-1.0, 0),
		Point(-0.5, 1.2),
		Point(-1, -0.6),
		Point(-2, 0.6),
		Point(-1, -0.6),
		Point(-0.5, 0),
		Point(-1.3, 0.6)
	};

	const int num_targets = G_N_ELEMENTS(targets);

	double dist_ball(const AI::HL::W::World &world, Point target){
		double dist =0;
		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			//	AI::HL::W::Player::Ptr p = world.friendly_team().get(i);
			Point temp = world.friendly_team().get(i)->position();
			dist += (target - temp).len();
		}
		return dist;
	}
}

BEGIN_PLAY(PassTest)
INVARIANT(true)
APPLICABLE(false)
DONE(false)
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()

Point target = targets[0];
double dist = dist_ball(world, target);

for(unsigned int i=0; i < num_targets; i++){
	Point temp = targets[i];
	if( dist_ball(world, temp) > dist){
		target = temp;
	}
}

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// passer
//roles[0].push_back(passer_shoot_target(world, target));
roles[0].push_back(passer_shoot_dynamic(world));

// ROLE 2
// passee
//roles[1].push_back(passee_move_target(world, target));
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


