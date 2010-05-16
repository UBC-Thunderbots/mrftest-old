#ifndef AI_TACTIC_MOVE_BETWEEN_ROBOTS_H
#define AI_TACTIC_MOVE_BETWEEN_ROBOTS_H

#include "ai/tactic/move_between.h"

//
// 
//
class move_between_robots : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<move_between_robots> ptr;

		//
		// Constructs a new move_between_robots tactic.
		//
		move_between_robots(player::ptr player, world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Sets the opponents' positions and orientations
		//
		void set_robots(robot::ptr robotA, robot::ptr robotB);

	protected:		

		move_between move_between_tactic;
		
		robot::ptr the_robot1;

		robot::ptr the_robot2;
};

#endif

