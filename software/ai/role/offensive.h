#ifndef AI_ROLE_OFFENSIVE_H
#define AI_ROLE_OFFENSIVE_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"
#include <vector>

/**
 * Gets the robots to go to their offensive positions.
 * Tries to receive the ball if defender or goalie has it.
 */
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

	protected:
		const world::ptr the_world;
        std::vector<tactic::ptr> the_tactics;
	
	private:

		// refactor this function?
		double get_distance_from_goal(int index) const;

		point calc_best(const std::vector<robot::ptr>& enemies) const;

		double calc_score(const std::vector<robot::ptr>& enemies, const point& pos) const;

		// Tells the robot to go towards the goal
		// refactor this in the future?
		void move_towards_goal(int index);
};

#endif

