#ifndef AI_HL_STP_WORLD_H
#define AI_HL_STP_WORLD_H

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			using namespace AI::HL::W;
			
			/**
			 * The max number of players we can assign roles
			 */
			const unsigned int TEAM_MAX_SIZE = 6; 
			
			/**
			 * The max speed the ball can be kicked
			 */
			const double BALL_MAX_SPEED = 8.0; 

			/**
			 * Distance from the penalty mark
			 */
			const double DIST_FROM_PENALTY_MARK = 0.4;
		}
	}
}

#endif

