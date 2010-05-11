#ifndef AI_ROLE_DEFENSIVE_H
#define AI_ROLE_DEFENSIVE_H

#include "ai/role.h"
#include "ai/tactic.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/chase_and_shoot.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include <vector>

//
// Gets the robots to go to their defensive positions.
//
class defensive : public role {
	public:
		//
		// A pointer to a defensive role.
		//
		typedef Glib::RefPtr<defensive> ptr;

		//
		// Constructs a new defensive role.
		//
		defensive(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

        //
        // Checks if the team has the ball
        //
        bool have_ball();

        //
        // Tells the robot to go towards the goal
        //
        void move_halfway_between_ball_and_our_goal(int index);

        //
        // Tells the robot to chase the ball
        //
        void chase_ball(int index);

        //
        // Tells the robot to block the ball
        //
        void block(int index);

        //
        // Gets the distance of the robot from the ball
        //
        double get_distance_from_ball(int index);

	// Kenneth:
        // This set the goalie robot in the defensive role. Note that this goalie is not included in the set_robots list! The role should handle this goalie separately!!
        void set_goalie(const player::ptr goalie);

        protected:
        std::vector<tactic::ptr> the_tactics;

        private:
        // note that this is a role in role, so the goalie role can still be developed independently.
        // Normally, the defensive role should just tick the goalie_role.
        // When the goalie has ball, this role can set the goalie to other tactics such as passing etc., but it should NEVER leave the goalie box.
        role::ptr goalie_role;
        player::ptr the_goalie;
};

#endif

