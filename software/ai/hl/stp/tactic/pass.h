#ifndef AI_HL_STP_TACTIC_PASS_H
#define AI_HL_STP_TACTIC_PASS_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * Passer Shoot
				 * Active Tactic
				 * Passer shoots towards the target region
				 */
				Tactic::Ptr passer_shoot_target(World world, Coordinate target);

				/**
				 * Passee Move
				 * Not Active Tactic
				 * Passee moves to intercept the moving ball, in the target region
				 */
				Tactic::Ptr passee_move_target(World world, Coordinate target);

				/**
				 * Passee Recieve Target
				 * Active Tactic
				 * Passee moves to intercept the moving ball, in the target region
				 * This one is an active tactic
				 */
				Tactic::Ptr passee_receive_target(World world, Coordinate target);

				/**
				 * Passer Shoot Dynamic
				 * Active Tactic
				 * Passer positioning and shoot for offensive purposes
				 * (objective is pass to passee and have it shoot at enemy goal).
				 */
				Tactic::Ptr passer_shoot_dynamic(World world);

				/**
				 * Passee Move Dynamic
				 * Not Active Tactic
				 * Passee positioning for offensive.
				 */
				Tactic::Ptr passee_move_dynamic(World world);

				/**
				 * Passee Recieve
				 * Active Tactic
				 * Passee moves to intercept / catch the moving ball
				 */
				Tactic::Ptr passee_receive(World world);
			}
		}
	}
}

#endif

