#ifndef AI_TACTIC_CHASE_AND_SHOOT_H
#define AI_TACTIC_CHASE_AND_SHOOT_H

#include "world/field.h"
#include "ai/tactic.h"
#include "ai/tactic/move.h"
#include "geom/point.h"
#include "ai/tactic/shoot.h"

//
// 
//
class chase_and_shoot : public tactic {
	public:
		typedef Glib::RefPtr<chase_and_shoot> ptr;
		//
		// Set a target that robot would like to take ball after gaining possesion
		//
		//void set_target(point target);

		//
		// Constructs a new chase tactic. 
		//
		chase_and_shoot(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:


 player::ptr the_player;
		move::ptr move_tactic;
		//our target is opponents net
		point target;
		//shoot::ptr shoot_tactic;
};

#endif

