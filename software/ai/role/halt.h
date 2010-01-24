/* ==================================================
Update History:

Name               Date               Remark
Kenneth            23 Jan 2010        Initial implementation

===================================================*/ 

#ifndef AI_ROLE_HALT_H
#define AI_ROLE_HALT_H

#include "ai/role.h"
#include "ai/tactic/move.h"

//
// Robots in this role should stop moving.
//
class halt : public role {
	public:
		//
		// A pointer to a halt role.
		//
		typedef Glib::RefPtr<halt> ptr;

		//
		// Constructs a new halt role.
		//
		halt(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:

        private:
                std::vector<move::ptr> tactics;
		
};

#endif

