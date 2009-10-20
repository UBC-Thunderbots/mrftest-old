#ifndef AI_TACTIC_MOVE_H
#define AI_TACTIC_MOVE_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/player.h"
#include "world/team.h"
#include "ai/tactic.h"
#include "ai/navigator.h"

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
		void update();

		//
		// Sets the target position for this move tactic
		//
		void set_position(const point& p);	

		//
		// Sets the target orientation for this move tactic
		//
		void set_orientation(const double& orientation);

	protected:		
		//
		// The navigator that chases.
		//
		navigator::ptr the_navigator;

		// Target position
		point target_position;

		// Target orientation
		double target_orientation;
};

#endif

