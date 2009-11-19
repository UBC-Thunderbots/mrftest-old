#ifndef AI_ROLE_PREPARE_KICKOFF_FRIENDLY_H
#define AI_ROLE_PREPARE_KICKOFF_FRIENDLY_H

#include "ai/role.h"
#include <vector>
#include "ai/tactic/move.h"
#include "ai/tactic.h"
#include "geom/point.h"

//
// Gets the robots to go to their prepare_kickoff_friendly positions.
//
class prepare_kickoff_friendly : public role {
	public:
		//
		// A pointer to a prepare_kickoff_friendly role.
		//
		typedef Glib::RefPtr<prepare_kickoff_friendly> ptr;

		//
		// Constructs a new prepare_kickoff_friendly role.
		//
		prepare_kickoff_friendly(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:
                std::vector<move::ptr> the_tactics;	
};

#endif

