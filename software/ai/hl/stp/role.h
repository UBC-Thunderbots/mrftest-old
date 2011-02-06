#ifndef AI_HL_STP_ROLE_H
#define AI_HL_STP_ROLE_H

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * A role is a sequence of tactics.
			 * This class allows one to refer players by their Role number.
			 * Because for a particular role,
			 * the tactic and player associated change all the time.
			 */
			class Role {
				public:
					/**
					 * Gets the player associated with this role.
					 */
					AI::HL::W::Player::Ptr evaluate() const;
			};
		}
	}
}

#endif

