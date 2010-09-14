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
				 * Gets the delay until the chicker is ready.
				 *
				 * \return the number of milliseconds until the chicker is ready to use.
				 */
				virtual unsigned int chicker_ready_time() const = 0;

				/**
				 * Gets the current speed the dribbler is spinning.
				 *
				 * \return the dribbler speed, in RPM.
				 */
				virtual unsigned int dribbler_speed() const = 0;
		};
	}
}

#endif

