#ifndef AI_FLAGS_H
#define AI_FLAGS_H

#include "ai/common/playtype.h"

namespace AI {
	namespace Flags {
		/**
		 * Flags indicating how robots comply with game rules.
		 * Flags are set by the Strategy and examined by the Navigator to determine which potential paths are legal.
		 */
		enum {
			/**
			 * Force robot to stay in play area.
			 */
			CLIP_PLAY_AREA = 0x0001,

			/**
			 * Avoid ball when stop is in play.
			 */
			AVOID_BALL_STOP = 0x0004,

			/**
			 * Avoid friendly defence area.
			 */
			AVOID_FRIENDLY_DEFENSE = 0x0008,

			/**
			 * Avoid enemy defence area.
			 */
			AVOID_ENEMY_DEFENSE = 0x0010,

			/**
			 * Stay in your own half.
			 */
			STAY_OWN_HALF = 0x0020,

			/**
			 * Neither goalie nor kicker.
			 * Stay away at certain boundary.
			 */
			PENALTY_KICK_FRIENDLY = 0x0040,

			/**
			 * Neither goalie nor kicker.
			 * Stay away at certain boundary.
			 */
			PENALTY_KICK_ENEMY = 0x0080,
		};

		/**
		 * Returns the correct flags for a common Player, i.e. a Player that is \em not a goalie or the free kicker.
		 *
		 * Assumes our team does not have the ball, unless the play type indicates otherwise.
		 * If this is not true, the caller should modify the flags appropriately.
		 *
		 * \param[in] pt the current play type.
		 *
		 * \return the appropriate flags.
		 */
		unsigned int calc_flags(AI::Common::PlayType::PlayType pt);
	}
}

#endif
