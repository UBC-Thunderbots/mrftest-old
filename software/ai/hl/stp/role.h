#ifndef AI_HL_STP_ROLE_H
#define AI_HL_STP_ROLE_H

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * This class allows various ways to refer to own players.
			 * By player, tactic, or role.
			 */
			class Role : public ByRef {
				public:
					typedef RefPtr<Role> Ptr;

					/**
					 * Gets the player associated with this role.
					 */
					virtual Player::Ptr evaluate() const = 0;

					/**
					 * A specific player.
					 */
					static Role::Ptr player(Player::Ptr player);

					/**
					 * A specific tactic.
					 */
					static Role::Ptr tactic(AI::HL::STP::Tactic::Tactic::Ptr tactic);

					/**
					 * Ordered by priority.
					 */
					static Role::Ptr role(unsigned int i);

					/**
					 * The goalie.
					 */
					static Role::Ptr goalie(World &world);

				protected:
					Role();

					~Role();
			};
		}
	}
}

#endif

