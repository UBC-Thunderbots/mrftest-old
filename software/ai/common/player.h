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
				 * \brief Checks if this robot has a chipper (a kicking device that can send the ball up into the air)
				 *
				 * \return \c true if a chipper is available, or \c false if not
				 */
				virtual bool has_chipper() const = 0;

				/**
				 * \brief Checks if this robot's autokick or autochip mechanism fired in the last tick.
				 *
				 * \return \c true if the kicker was fired due to the autokick or autochip mechanism since the last tick, or \c false if not.
				 */
				virtual bool autokick_fired() const = 0;
		};
	}
}

#endif

