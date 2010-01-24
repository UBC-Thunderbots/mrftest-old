/* ==================================================
Update History:

Name               Date               Remark
Kenneth            23 Jan 2010        Initial implementation

===================================================*/ 

#ifndef AI_ROLE_STOP_H
#define AI_ROLE_STOP_H

#include "ai/role.h"
#include "ai/tactic/move.h"

//
// Robots in this role should stay away from the ball for a certain distance.
//
class stop : public role {
	public:
		//
		// A pointer to a stop role.
		//
		typedef Glib::RefPtr<stop> ptr;

		//
		// Constructs a new stop role.
		//
		stop(ball::ptr ball, field::ptr field, controlled_team::ptr team);

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
                static const double DIST_AWAY_FROM_BALL = 0.5;
                static const double PI = 3.14159;
                //point our_goal;
		//point centre;
		std::vector<move::ptr> tactics;
                std::vector<point> points;
                void fix_if_out_of_bound(point& pt);
};

#endif

