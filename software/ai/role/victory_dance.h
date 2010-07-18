#ifndef AI_ROLE_VICTORY_DANCE_H
#define AI_ROLE_VICTORY_DANCE_H

#include "ai/role/role.h"
#include "ai/tactic/dance.h"

//
// Gets the robots to go to their victory dance positions.
//
class VictoryDance : public Role {
	public:
		//
		// A pointer to a VictoryDance Role.
		//
		typedef Glib::RefPtr<VictoryDance> ptr;

		//
		// Constructs a new VictoryDance Role.
		//
		VictoryDance();

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:
        std::vector<Dance::ptr> the_tactics;
		
};

#endif

