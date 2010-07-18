#ifndef AI_ROLE_DEFENSIVE_H
#define AI_ROLE_DEFENSIVE_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"
#include <vector>

// TODO: Ignore this message
// Kenneth: PLEASE READ THIS BEFORE UPDATING.
// 1) the_goalie is guaranteed to be non-empty
// 2) the_robots may be empty(i.e. the defensive role only has one goalie, which is stored separately). In this case the defensive role should just tick the Goalie role and do nothing else.
// 3) If the goalie has ball and there are at least two robots on the field, the the_robots has at least one robot.

/**
 * While the goalie blocks the  ball's direct line of sight.
 * Defenders block enemy robots.
 * Rules:
 * When goalie is in possesion of a ball,
 * tries to be in line of sight of the goalie to receive the ball.
 * If the defensive is in possesion of a ball,
 * tries to pass to a robot not in defensive role closest to an enemy goal.
 * If not possible, passes to a defender or goalie (randomly).
 */
class Defensive : public Role {
	public:
		//
		// A pointer to a Defensive Role.
		//
		typedef Glib::RefPtr<Defensive> ptr;

		//
		// Constructs a new Defensive Role.
		//
		Defensive(World::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:

		/**
		 * Calculate points which should be used to defend from
		 * enemy robots. Should be adjusted to take into account
		 * of the rules and number of robots in this Role.
		 */
		std::vector<Point> calc_block_positions() const;

		const World::ptr the_world;

		std::vector<Tactic::ptr> tactics;
};

#endif

