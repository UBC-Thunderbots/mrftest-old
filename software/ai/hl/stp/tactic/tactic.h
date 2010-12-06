#ifndef AI_HL_STP_TACTICS_H
#define AI_HL_STP_TACTICS_H

#include "ai/hl/world.h"
#include "util/byref.h"
#include "util/registerable.h"

namespace AI {
	namespace HL {
		namespace STP {

			/**
			 * A tactic is a layer in the STP paradigm.
			 *
			 * A stateless tactic is always preferred for simplicity.
			 * However, tactic can be stateful.
			 * In such a case, a play will have to store the tactic.
			 */
			class Tactic : public ByRef {
				public:
					typedef RefPtr<Tactic> Ptr;

					Tactic(AI::HL::W::World& world);

					~Tactic();

					/**
					 * Scoring function
					 * to indicate how preferable this particular player is.
					 *
					 * \return between 1 and 0, indicating the preference.
					 */
					virtual double score(AI::HL::W::Player::Ptr player) const = 0;

					/**
					 * Drive some actual players.
					 */
					virtual void tick(AI::HL::W::Player::Ptr player) = 0;

				protected:
					AI::HL::W::World& world;
			};

			///////////////////////////////////////////////////////////////////
			// STANDARD STATELESS TACTICS

			/**
			 * Goto some place.
			 */
			Tactic::Ptr move(AI::HL::W::World& world, const Point dest);

			/**
			 * A standard lone goalie tactic.
			 */
			Tactic::Ptr defend_goal(AI::HL::W::World& world);

			/**
			 * Move the ball away from our own goal at all cost.
			 */
			Tactic::Ptr repel(AI::HL::W::World& world);

			/**
			 * Defends against a specified enemy.
			 */
			Tactic::Ptr block(AI::HL::W::World& world, AI::HL::W::Robot::Ptr robot);

			/**
			 * Shoot for the enemy goal.
			 */
			Tactic::Ptr shoot(AI::HL::W::World& world);

			/**
			 * Shoot a specified target.
			 */
			Tactic::Ptr shoot(AI::HL::W::World& world, const Point target);

			/**
			 * Go for the ball.
			 */
			Tactic::Ptr chase(AI::HL::W::World& world);

			/**
			 * Nothing LOL.
			 */
			Tactic::Ptr idle(AI::HL::W::World& world);

			/**
			 * Passing requires some a passer and passee.
			 * To use this correctly, create a reference pointer to this instance.
			 */
			class Pass : public ByRef {
				public:
					typedef RefPtr<Pass> Ptr;

					Pass(AI::HL::W::World& world);

					/**
					 * Returns a tactic instance for passer.
					 */
					Tactic::Ptr passer();

					/**
					 * Returns a tactic instance for passee.
					 */
					Tactic::Ptr passee();

				private:

					/**
					 * The actual work is done in this function.
					 */
					void tick();
			};
		}
	}
}

#endif

