#ifndef AI_TACTIC_MOVE_H
#define AI_TACTIC_MOVE_H

#include "ai/navigator/robot_navigator.h"
#include "ai/tactic/tactic.h"

//
// Just a wrapper around the move function in player.
//
class move : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<move> ptr;

		//
		// Constructs a new move tactic.
		//
		move(player::ptr player, world::ptr world);

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

		const player::ptr the_player;

		// Target position
		point target_position;

		bool avoid_ball;
		
		// The navigator that moves
		robot_navigator the_navigator;
};

#endif

