#ifndef AI_HL_FREE_KICK_FRIENDLY_H
#define AI_HL_FREE_KICK_FRIENDLY_H

#include "ai/hl/world.h"
#include <vector>

namespace AI {
	namespace HL {
		/**
		 * Gets the robots to go to their penalty_enemy positions.
		 */
		class DirectFreeKickFriendly {
			public:
				/**
				 * A pointer to a free_kick_friendly role.
				 */
				typedef RefPtr<DirectFreeKickFriendly> ptr;

				/**
				 * Constructs a new free_kick_friendly role.
				 *
				 * \param[in] w the world.
				 */
				DirectFreeKickFriendly(W::World &w);

				/**
				 * Sets the kicker
				 *
				 * \param[in] k kicker.
				 */
				void set_players(W::Player::Ptr &k) {
					kicker = k;
				}

				/**
				 * Runs the AI for one time tick.
				 */
				void tick();

			protected:
				W::World &world;

				W::Player::Ptr kicker;
		};

		class IndirectFreeKickFriendly {
			public:
				/**
				 * A pointer to a free_kick_friendly role.
				 */
				typedef RefPtr<IndirectFreeKickFriendly> ptr;

				/**
				 * Constructs a new free_kick_friendly role.
				 *
				 * \param[in] w the world.
				 */
				IndirectFreeKickFriendly(W::World &w);

				/**
				 * Sets the kicker
				 *
				 * \param[in] k kicker.
				 */
				void set_players(W::Player::Ptr &k) {
					kicker = k;
				}

				/**
				 * Runs the AI for one time tick.
				 */
				void tick();

			protected:
				W::World &world;

				W::Player::Ptr kicker;
		};
	}
}

#endif

