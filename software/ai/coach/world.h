#ifndef AI_COACH_WORLD_H
#define AI_COACH_WORLD_H

#include "ai/common/world.h"
#include "ai/hl/strategy.h"
#include "util/property.h"
#include <cstddef>

namespace AI {
	namespace Coach {
		namespace W {
			namespace PlayType = AI::Common::PlayType;

			/**
			 * The friendly team.
			 */
			class FriendlyTeam : public AI::Common::Team {
			};

			/**
			 * The enemy team.
			 */
			class EnemyTeam : public AI::Common::Team {
			};

			/**
			 * The world, as seen by a Coach.
			 */
			class World {
				public:
					/**
					 * Returns the friendly team.
					 *
					 * \return the friendly team.
					 */
					virtual FriendlyTeam &friendly_team() = 0;

					/**
					 * Returns the enemy team.
					 *
					 * \return the enemy team.
					 */
					virtual EnemyTeam &enemy_team() = 0;

					/**
					 * Returns the current play type.
					 *
					 * \return the current play type.
					 */
					virtual const Property<PlayType::PlayType> &playtype() const = 0;

					/**
					 * Returns or allows setting the current strategy.
					 *
					 * \return the current strategy.
					 */
					virtual Property<AI::HL::Strategy::Ptr> &strategy() = 0;

					/**
					 * Sets the current strategy from a factory.
					 *
					 * \param s the new StrategyFactory to use.
					 */
					virtual void strategy(AI::HL::StrategyFactory *s) = 0;
			};
		}
	}
}

#endif

