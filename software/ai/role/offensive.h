#ifndef AI_ROLE_OFFENSIVE_H
#define AI_ROLE_OFFENSIVE_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"
#include <vector>

namespace {
	// temporary
	const int GRIDY = 25;
	const int GRIDX = 25;
}

/**
 * Gets the robots to go to their offensive positions.
 * Tries to receive the ball if defender or goalie has it.
 * If in possesion of ball, tries to find best positions to shoot and score.
 */
class Offensive : public Role {
	public:
		//
		// A pointer to a Offensive Role.
		//
		typedef Glib::RefPtr<Offensive> ptr;

		//
		// Constructs a new Offensive Role.
		//
		Offensive(World::ptr world);

		//
		// Runs the AI for one time tick without drawing.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void players_changed();

	protected:

		/**
		 * Assume that role has the ball.
		 * Find where to position the robot so that it has the greatest chance of shooting.
		 * The enemy position is provided as vector so we can add imaginary enemies.
		 * If no position is valid, will simply choose the middle of the field.
		 */
		Point calc_position_best(const std::vector<Point>& enemypos, const std::vector<Point>& dontblock);

		/// The scoring function for having the robot in the particular position.
		double scoring_function(const std::vector<Point>& enemypos, const Point& pos, const std::vector<Point>& dontblock) const;

		/// Calculates n best positions to place the robots.
		std::vector<Point> calc_position_best(const unsigned int n);

		// refactor this function?
		double get_distance_from_goal(int index) const;

		const World::ptr the_world;

		std::vector<Tactic::ptr> tactics;

		bool okaygrid[GRIDX][GRIDY];

};

#endif

