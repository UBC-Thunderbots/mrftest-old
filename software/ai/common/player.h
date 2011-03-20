#ifndef AI_COMMON_PLAYER_H
#define AI_COMMON_PLAYER_H

#include "ai/common/robot.h"
#include "util/byref.h"

namespace AI {
	namespace Common {
		/**
		 * The common functions available on a player in all layers, not including those in Robot.
		 */
		class Player : public virtual ByRef {
			public:
				/**
				 * Returns whether or not this player has the ball.
				 *
				 * \return \c true if this player has the ball, or \c false if not.
				 */
				virtual bool has_ball() const = 0;

				/**
				 * Checks if this robot's chicker is ready to use.
				 *
				 * \return \c true if ready, or \c false if not.
				 */
				virtual bool chicker_ready() const = 0;
		};
	}
}

#endif

