#ifndef AI_ROLE_OFFENSIVE_H
#define AI_ROLE_OFFENSIVE_H

#include "ai/role/role.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/chase_and_shoot.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/pass.h"
#include <vector>

//
// Gets the robots to go to their offensive positions.
//
class offensive : public role {
	public:
		//
		// A pointer to a offensive role.
		//
		typedef Glib::RefPtr<offensive> ptr;

		//
		// Constructs a new offensive role.
		//
		offensive(world::ptr world);

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
        void move_towards_goal(int index);

        //
        // Tells the robot to shoot at the goal
        //
        void shoot_at_goal(int index);

        //
        // Tells the robot to chase the ball
        //
        void chase_ball(int index);

        void pass_ball(int index, int receiver);

        //
        // Gets the distance of the robot from the enemy's goal
        //
        double get_distance_from_goal(int index);

	protected:
		const world::ptr the_world;
        std::vector<tactic::ptr> the_tactics;
		
};

#endif

