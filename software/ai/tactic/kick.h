#ifndef AI_TACTIC_KICK_H
#define AI_TACTIC_KICK_H

#include "ai/tactic/tactic.h"
#include "ai/world/world.h"

/**
 * Tactic for FORCED KICKING!!!
 * By default, kicks at full strength.
 * If not set a target, will just kick in whatever direction it is facing.
 */
class Kick : public Tactic {
	public:
		//
		// Constructs a new Kick Tactic.
		//
		Kick(RefPtr<Player> player, RefPtr<World> world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		/**
		 * Sets the target position for this Kick Tactic.
		 */
		void set_target(const Point& p) {
			kick_target = p;
		}

		/**
		 * Enables and sets the strength of the chipping.
		 * Value between 0 and 1.
		 */
		void set_chip(const double& s = 1.0) {
			strength = s;
			should_chip = true;
		}

		/**
		 * Enables and sets the strength of the chipping.
		 * Value between 0 and 1.
		 */
		void set_kick(const double& k = 1.0) {
			strength = k;
			should_chip = false;
		}

	protected:
		const RefPtr<World> the_world;

		// True to chip instead of kicking.
		bool should_chip;
		double strength;

		// Target position
		Point kick_target;
};

#endif

