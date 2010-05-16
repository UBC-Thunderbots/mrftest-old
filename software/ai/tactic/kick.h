#ifndef AI_TACTIC_KICK_H
#define AI_TACTIC_KICK_H

#include "ai/tactic/turn.h"

/**
 * Tactic for doing either chip or kick.
 * By default, kicks at full strength.
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
		kick(player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target position for this kick tactic.
		 */
		void set_target(const point& p) {
			the_target = p;
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
		const player::ptr the_player;

	protected:		

		turn turn_tactic;

		// Target position
		point the_target;

		// Holds if this tactic should chip the ball rather than simply kicking it
		bool should_chip;

		double chip_strength;

		double kick_strength;
};

#endif

