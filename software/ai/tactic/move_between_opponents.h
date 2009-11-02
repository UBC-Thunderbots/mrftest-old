#ifndef AI_TACTIC_MOVE_BETWEEN_OPPONENTS_H
#define AI_TACTIC_MOVE_BETWEEN_OPPONENTS_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/player.h"
#include "world/team.h"
#include "ai/tactic.h"
#include "ai/tactic/move.h"

//
// Just a wrapper around the move_between_opponents function in player.
//
class move_between_opponents : public tactic {
	public:
		//
		// A pointer to a move tactic.
		//
		typedef Glib::RefPtr<move_between_opponents> ptr;

		//
		// Constructs a new move_between_opponents tactic.
		//
		move_between_opponents(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void update();

		//
		// Sets the opponents' positions and orientations
		//
		void set_opponents(robot::ptr oppA, robot::ptr oppB);

		//
		// Calculates the position
		//
		point calculate_position();

		//
		// Calculates the orientation
		double calculate_orientation();

	protected:		

		move::ptr move_tactic;
		
		robot::ptr opponentA;

		robot::ptr opponentB;
};

#endif

