#ifndef AI_TACTIC_MOVE_BETWEEN_H
#define AI_TACTIC_MOVE_BETWEEN_H

#include "ai/tactic/move.h"

//
// 
//
class move_between : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<move_between> ptr;

		//
		// Constructs a new move between tactic. 
		//
		move_between(player::ptr player, world::ptr world);

		//
		// Sets the two points in which to move between
		//
		void set_points(const point& p1, const point& p2);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:

		move move_tactic;

		// The two points in which to move between
		point the_point1, the_point2;

		point calculate_position();

};

#endif

