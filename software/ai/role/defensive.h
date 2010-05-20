#ifndef AI_ROLE_DEFENSIVE_H
#define AI_ROLE_DEFENSIVE_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"
#include <vector>

// TODO: Ignore this message
// Kenneth: PLEASE READ THIS BEFORE UPDATING.
// 1) the_goalie is guaranteed to be non-empty
// 2) the_robots may be empty(i.e. the defensive role only has one goalie, which is stored separately). In this case the defensive role should just tick the goalie role and do nothing else.
// 3) If the goalie has ball and there are at least two robots on the field, the the_robots has at least one robot.

/**
 * While the goalie blocks the  ball's direct line of sight.
 * Defenders block enemy robots.
 * Rules:
 * When goalie is in possesion of a ball, tries to be in line of sight of the goalie to receive the ball.
 * If the defensive is in possesion of a ball, tries to pass to a robot not in defensive role closest to an enemy goal.
 * If not possible, passes to a defender or goalie (randomly).
 */
class defensive : public role {
	public:
		//
		// A pointer to a defensive role.
		//
		typedef Glib::RefPtr<defensive> ptr;

		//
		// Constructs a new defensive role.
		//
		defensive(world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

		/// TODO: remove this in the future?
		void move_halfway_between_ball_and_our_goal(int index);

		/// TODO: remove this in the future?
		void set_goalie(const player::ptr goalie);

	protected:

		/**
		 * Calculate points which should be used to defend from enemy robots.
		 * Should be adjusted to take into account of the rules and number of robots in this role.
		 */
		std::vector<point> calc_block_positions() const;

		std::vector<tactic::ptr> the_tactics;

	private:
		const world::ptr the_world;

		// TODO: get rid of this message
		// note that this is a role in role, so the goalie role can still be developed independently.
		// Normally, the defensive role should just tick the goalie_role.
		// When the goalie has ball, this role can set the goalie to other tactics such as passing etc., but it should NEVER leave the goalie box.
		role::ptr goalie_role;
		player::ptr the_goalie;

		/**
		 * This should be removed in the future.
		 */
		void tick_goalie();
};

#endif

