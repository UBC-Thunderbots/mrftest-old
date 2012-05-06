#ifndef AI_COMMON_PLAYER_H
#define AI_COMMON_PLAYER_H

#include "ai/common/robot.h"

namespace AI {
	namespace Common {
		/**
		 * The common functions available on a player in all layers, not including those in Robot.
		 */
		class Player {
			public:
				/**
				 * \brief Returns whether or not the player is alive.
				 *
				 * \return \c true if the player is alive, or \c false if the radio cannot communicate with the player.
				 */
				virtual bool alive() const = 0;

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
				 * \return \c false
				 *
				 * \deprecated No robots have directional kickers; this function always returns false.
				 */
				bool kicker_directional() const __attribute__((deprecated("No robots have directional kickers; this function always returns false.")));

				/**
				 * \brief Checks if this robot's autokick mechanism fired in the last tick.
				 *
				 * \return \c true if the kicker was fired due to the autokick mechanism since the last tick, or \c false if not.
				 */
				virtual bool autokick_fired() const = 0;
		};
	}
}



inline bool AI::Common::Player::kicker_directional() const {
	return false;
}

#endif

