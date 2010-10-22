#ifndef AI_HL_PENALTY_FRIENDLY_H
#define AI_HL_PENALTY_FRIENDLY_H

#include "ai/hl/world.h"
#include "ai/hl/strategy.h"
#include <vector>

namespace AI {
	namespace HL {
		/**
		 * Gets the robots to go to their penalty_friendly positions.
		 */
		class PenaltyFriendly {
			public:

				/**
				 * Constructs a new penalty_friendly role.
				 *
				 * \param[in] w the world.
				 */
				PenaltyFriendly(W::World &w);

				/**
				 * Sets all the players
				 *
				 * \param[in] p players.
				 *
				 */
				void set_players(const std::vector<W::Player::Ptr> &p) {
					players = p;
				}

				/**
				 * This function can only be called ONCE per tick.
				 */
				void tick();

			protected:
				W::World &world;

				std::vector<W::Player::Ptr> players;

				/**
				 * The distance between the penalty mark and the mid point of the two goal posts as described in the rules.
				 */
				const static double PENALTY_MARK_LENGTH;

				/**
				 * The distance between the baseline and the line behind which other robots may stand.
				 */
				const static double RESTRICTED_ZONE_LENGTH;

				/**
				 * Maximum number of robots that can be assigned to this role.
				 */
				const static unsigned int NUMBER_OF_READY_POSITIONS = 4;

				/**
				 * The positions that the robots should move to for this role.
				 */
				Point ready_positions[NUMBER_OF_READY_POSITIONS];
		};
	}
}

#endif

