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

				/**
				 * \brief Checks if this robot's kicker is able to kick on an angle.
				 *
				 * \return \c true if the kicker can kick on an angle, or \c false if the kicker can only kick straight.
				 */
				virtual bool kicker_directional() const = 0;

				/**
				 * \brief Checks if this robot's autokick mechanism fired in the last tick.
				 *
				 * \return \c true if the kicker was fired due to the autokick mechanism since the last tick, or \c false if not.
				 */
				virtual bool autokick_fired() const = 0;
		};
	}
}

#endif

