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
		// Constructs a new Chase Tactic. 
		//
		Chase(RefPtr<Player> player, RefPtr<World> world);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:
		const RefPtr<World> the_world;
		RobotNavigator navi;
};

#endif

