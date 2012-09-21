#ifndef AI_HL_STP_ROLE_H
#define AI_HL_STP_ROLE_H

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "util/noncopyable.h"
#include <utility>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * \brief This class allows various ways to refer to own players.
			 *
			 * By player, tactic, or role.
			 */
			class Role : public NonCopyable {
				public:
					/**
					 * \brief A pointer to a Role.
					 */
					typedef std::shared_ptr<Role> Ptr;

					/**
					 * \brief Gets the player associated with this role.
					 */
					virtual Player::Ptr evaluate() const = 0;

					/**
					 * \brief A specific player.
					 */
					static Role::Ptr player(Player::Ptr player);

					/**
					 * \brief A specific tactic.
					 */
					static Role::Ptr tactic(const AI::HL::STP::Tactic::Tactic::Ptr &tactic);

					/**
					 * \brief Ordered by priority.
					 */
					static Role::Ptr role(unsigned int i);

					/**
					 * \brief The goalie.
					 */
					static Role::Ptr goalie(World world);

				protected:
					Role();
			};
		}
	}
}

#endif

