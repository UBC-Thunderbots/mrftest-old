#ifndef AI_TACTIC_PATROL_H
#define AI_TACTIC_PATROL_H

#include "ai/world/world.h"
#include "ai/tactic/tactic.h"
#include "ai/navigator/robot_navigator.h"

/**
 * This assumes that the role does not get recreated and that this tactic persists for the duration of the role using this tactic.
 */
class patrol : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<patrol> ptr;

		/**
		 * Standard constructor.
		 */
		patrol(player::ptr player, world::ptr world);

		/**
		 * Most usage of move tactic only sets position and should thus justify existence of this overloaded constructor.
		 * \param position1 The first position for patrol
		 * \param position2 The second position for patrol
		 */
		patrol(player::ptr player, world::ptr world, const unsigned int& flags, const point& t1, const point& t2);

		/**
		 * Set the targets for the patrol.
		 */
		void set_targets(const point& t1, const point& t2) {
			target1 = t1;
			target2 = t2;
			target_initialized = true;
		}

		//
		// Runs the AI for one time tick.
		//
		void tick();

	protected:		
		robot_navigator navi;
		point target1, target2;
		bool target_initialized;
};

#endif

