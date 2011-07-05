#ifndef AI_HL_STP_TACTIC_PASS_H
#define AI_HL_STP_TACTIC_PASS_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {

				/**
				 * Passer shoots towards the target region
				 */
				Tactic::Ptr passer_shoot_target(const World &world, Coordinate target);

				/**
				 * Passee moves to intercept the moving ball, in the target region
				 */
				Tactic::Ptr passee_move_target(const World &world, Coordinate target);

				/**
				 * Passee moves to intercept the moving ball, in the target region
				 * This one is an active tactic
				 */
				Tactic::Ptr passee_receive_target(const World &world, Coordinate target);

				/**
				 * Passer positioning and shoot for offensive purposes
				 * (objective is pass to passee and have it shoot at enemy goal).
				 */
				Tactic::Ptr passer_shoot_dynamic(const World &world);

				/**
				 * Passee positioning for offensive.
				 */
				Tactic::Ptr passee_move_dynamic(const World &world);

				/**
				 * Passee moves to intercept / catch the moving ball
				 * This one is an active tactic
				 */
				Tactic::Ptr passee_receive(const World &world);

				/**
				 * Just toss the ball to someone in front.
				 */
				Tactic::Ptr passer_simple(const World &world);

				/**
				 * Stay at position and wait for pass.
				 * number: first passee is 0, second is 1 etc
				 */
				Tactic::Ptr passee_simple(const World &world, unsigned number);
			}
		}
	}
}

#endif

