#ifndef AI_TACTIC_TURN_H
#define AI_TACTIC_TURN_H

#include "ai/tactic/tactic.h"
#include "ai/world/player.h"
#include "geom/point.h"



//
// 
//
class turn : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<turn> ptr;

		//
		// Constructs a new block tactic. 
		//
		turn(player::ptr player);

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
		const player::ptr the_player;
		
		//
		// Returns the change in angle between the current orientation and the desired orientation.
		//
		double d_angle();
};

#endif

