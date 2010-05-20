#ifndef AI_ROLE_EXECUTE_PENALTY_ENEMY_H
#define AI_ROLE_EXECUTE_PENALTY_ENEMY_H

#include "ai/role/role.h"
#include "ai/tactic/move.h"

/**
 * Gets the robots to go to their execute_penalty_enemy positions.
 */
class execute_penalty_enemy : public role {
	public:
		/**
		 * A pointer to a execute_penalty_enemy role.
		 */
		typedef Glib::RefPtr<execute_penalty_enemy> ptr;

		/**
		 * Constructs a new execute_penalty_enemy role.
		 *
		 * \param world the world
		 */
		execute_penalty_enemy(world::ptr world);

		/**
		 * Runs the AI for one time tick.
		 */
		void tick();

		/**
		 * Handles changes to the robot membership.
		 */
		void robots_changed();

	private:
		/**
		 * Patrols between the starting position and ending position.
		 */
		void patrol();
	
		/**
		 * Returns whether the shooter has made a move.
		 */
		bool detect_enemy_movement();

		const world::ptr the_world;

		/**
		 * The distance between the penalty mark and the mid point of the two goal posts as described in the rules.
		 */
		const static double penalty_mark_length = 0.45;

		/**
		 * The distance between the baseline and the line behind which other robots may stand.
		 */
		const static double restricted_zone_length = 0.85;

		/**
		 * The starting position.
		 */
		const point starting_position;

		/**
		 * The ending position.
		 */
		const point ending_position;

		/**
		 * The tactic to move to the starting position.
		 */
		move::ptr move_to_start;

		/**
		 * The tactic to move to the ending position.
		 */
		move::ptr move_to_end;

		/**
		 * Whether the goalie should patrol or try to save the ball.
		 */
		bool should_patrol;

		/**
		 * Used for patrol to see if the robot should move to start or end.
		 */
		bool should_go_to_start;

		/**
		 * The shooter has begun shooting - move to the other corner as fast as possible.
		 */
		bool enemy_moved;

		/**
		 * The robot used for the goalie (the only robot assigned to this role.
		 */
		player::ptr the_goalie;
		
		/**
		 * The enemy shooter.
		 */
		robot::ptr the_shooter;

		/**
		* The last orientation of the shooter.
		*/
		double last_orientation;
};

#endif

