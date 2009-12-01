#ifndef AI_TACTIC_TURN_H
#define AI_TACTIC_TURN_H

#include "ai/tactic.h"

//
// 
//
class turn : public tactic {
	public:
		//
		// A pointer to a turn tactic.
		//
		typedef Glib::RefPtr<turn> ptr;

		//
		// Constructs a new block tactic. 
		//
		turn(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Sets the direction for the turn tactic. Takes in a point and turns towards that point.
		//
		void set_direction(const point& dir);	

		//
		// Returns true if the angle for which we need to turn is smaller than the tolerance.
		//
		bool is_turned(const double& tol);

	protected:

		point the_direction;				

	private:
		
		//
		// Returns the change in angle between the current orientation and the desired orientation.
		//
		double d_angle();
};

#endif

