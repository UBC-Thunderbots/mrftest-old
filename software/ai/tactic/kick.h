#ifndef AI_TACTIC_KICK_H
#define AI_TACTIC_KICK_H

#include "ai/tactic/tactic.h"
#include "ai/navigator/robot_navigator.h"

#warning obselete? superceded by pass and shoot

/**
 * Tactic for doing either chip or kick.
 * By default, kicks at full strength.
 * Right now, for the sake of testing, the robot is permitted to kick the air.
 */
class kick : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<kick> ptr;

		//
		// Constructs a new kick tactic.
		//
		kick(player::ptr player, world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target position for this kick tactic.
		 */
		void set_target(const point& p) {
			kick_target = p;
			target_initialized = true;
		}

		/**
		 * Enables and sets the strength of the chipping.
		 * Value between 0 and 1.
		 */
		void set_chip(const double& s = 1.0) {
			chip_strength = s;
			should_chip = true;
		}

		/**
		 * Enables and sets the strength of the chipping.
		 * Value between 0 and 1.
		 */
		void set_kick(const double& k = 1.0) {
			kick_strength = k;
			should_chip = false;
		}

	protected:
		const world::ptr the_world;
		robot_navigator navi;

		// True to chip instead of kicking.
		bool should_chip;
		double chip_strength;
		double kick_strength;

		// Target position
		bool target_initialized;
		point kick_target;
};

#endif

