#ifndef AI_TACTIC_CHASE_H
#define AI_TACTIC_CHASE_H

#include "ai/tactic/tactic.h"
#include "ai/navigator/robot_navigator.h"

/**
 * Chase the ball.
 */
class Chase : public Tactic {
	public:
		//
		// A pointer to this Tactic.
		//
		typedef RefPtr<Chase> ptr;

		//
		// Constructs a new Chase Tactic. 
		//
		Chase(Player::ptr player, World::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:
		const World::ptr the_world;
		RobotNavigator navi;
};

#endif

