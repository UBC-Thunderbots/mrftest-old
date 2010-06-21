#ifndef AI_ROLE_OFFENSIVE_H
#define AI_ROLE_OFFENSIVE_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"
#include <vector>

namespace {
	// temporary
	const int GRIDY = 50;
	const int GRIDX = 50;
}

/**
 * Gets the robots to go to their offensive positions.
 * Tries to receive the ball if defender or goalie has it.
 * If in possesion of ball, tries to find best positions to shoot and score.
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
		// Runs the AI for one time tick without drawing.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:

		/**
		 * Assume that role has the ball.
		 * Find where to position the robot so that it has the greatest chance of shooting.
		 * The enemy position is provided as vector so we can add imaginary enemies.
		 * If no position is valid, will simply choose the middle of the field.
		 */
		point calc_position_best(const std::vector<point>& enemypos, const std::vector<point>& dontblock);

		/// The scoring function for having the robot in the particular position.
		double scoring_function(const std::vector<point>& enemypos, const point& pos, const std::vector<point>& dontblock) const;

		/// Calculates n best positions to place the robots.
		std::vector<point> calc_position_best(const unsigned int n);

		// refactor this function?
		double get_distance_from_goal(int index) const;

		const world::ptr the_world;

		std::vector<tactic::ptr> tactics;

		bool okaygrid[GRIDX][GRIDY];

};

#endif

