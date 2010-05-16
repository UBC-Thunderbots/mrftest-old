#ifndef AI_ROLE_GOALIE_H
#define AI_ROLE_GOALIE_H

#include "ai/role/role.h"

//
// Gets the robots to go to their goalie positions.
//
class goalie : public role {
	public:
		//
		// A pointer to a goalie role.
		//
		typedef Glib::RefPtr<goalie> ptr;

		//
		// Constructs a new goalie role.
		//
		goalie(world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

                //
                // Before calling this, the goalie shouldn't react to the ball's position.
                // After, it should.
                //
                void start_play();

	protected:

        private:
				const world::ptr the_world;
                bool started;
                point default_pos;
                point centre_of_goal;
                static const double STANDBY_DIST = 0.2;		
};

#endif

