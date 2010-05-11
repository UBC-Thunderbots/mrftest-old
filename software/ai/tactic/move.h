#ifndef AI_TACTIC_MOVE_H
#define AI_TACTIC_MOVE_H

#include "ai/navigator.h"
#include "ai/tactic.h"

//
// Just a wrapper around the move function in player.
//
class move : public tactic {
	public:
		//
		// A pointer to a move tactic.
		//
		typedef Glib::RefPtr<move> ptr;

		//
		// Constructs a new move tactic.
		//
		move(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void tick();
		
		//
		//make the move tactic avoid the ball
		//
		void set_avoid_ball(bool avoid);
		
		//
		// Sets the target position for this move tactic
		//
		void set_position(const point& p);	

	protected:		

		// The navigator that moves
		navigator::ptr the_navigator;

		// Target position
		point target_position;

		bool avoid_ball;
};

#endif

