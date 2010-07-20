#ifndef AI_TACTIC_PATROL_H
#define AI_TACTIC_PATROL_H

#include "ai/world/world.h"
#include "ai/tactic/tactic.h"
#include "ai/navigator/robot_navigator.h"

/**
 * This assumes that the role does not get recreated and that this Tactic persists for the duration of the role using this Tactic.
 */
class Patrol : public Tactic {
	public:
		/**
		 * Standard constructor.
		 */
		Patrol(RefPtr<Player> player, RefPtr<World> world);

		/**
		 * Most usage of Move Tactic only sets position and should thus justify existence of this overloaded constructor.
		 * \param position1 The first position for patrol
		 * \param position2 The second position for patrol
		 */
		Patrol(RefPtr<Player> player, RefPtr<World> world, const unsigned int& flags, const Point& t1, const Point& t2);

		/**
		 * Set the targets for the patrol.
		 */
		void set_targets(const Point& t1, const Point& t2) {
			target1 = t1;
			target2 = t2;
			target_initialized = true;
		}

		//
		// Runs the AI for one time tick.
		//
		void tick();

	protected:		
		RobotNavigator navi;
		Point target1, target2;
		bool target_initialized;
};

#endif

