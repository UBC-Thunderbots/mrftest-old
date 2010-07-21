#ifndef AI_TACTIC_MOVE_BETWEEN_H
#define AI_TACTIC_MOVE_BETWEEN_H

#include "ai/tactic/move.h"

//
// 
//
class MoveBetween : public Tactic {
	public:
		//
		// A pointer to this Tactic.
		//
		typedef RefPtr<MoveBetween> ptr;

		//
		// Constructs a new move between Tactic. 
		//
		MoveBetween(Player::ptr player, World::ptr world);

		//
		// Sets the two points in which to move between
		//
		void set_points(const Point& p1, const Point& p2);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:

		Move move_tactic;

		// The two points in which to move between
		Point the_point1, the_point2;

		Point calculate_position();

	private:
		bool is_initialized;
};

#endif

