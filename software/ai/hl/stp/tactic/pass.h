#ifndef AI_HL_STP_TACTIC_PASS_H
#define AI_HL_STP_TACTIC_PASS_H

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {
				/**
				 * The STP suggested method for passing.
				 *
				 * Instructions for writing play:
				 * 1 passer use passer_ready and passee use passee_ready.
				 * 2 passer use passer_shoot and passee use passee_receive.
				 *
				 * \param [in] pos where the passer should be.
				 *
				 * \param [in] target where the passee should be.
				 */
				Tactic::Ptr passer_ready(const AI::HL::W::World &world, AI::HL::STP::Coordinate pos, AI::HL::STP::Coordinate target);

				/**
				 * The STP suggested method for passing.
				 * See instructions in passer_ready_position.
				 *
				 * \param [in] pos where the passee should be.
				 */
				Tactic::Ptr passee_ready(const AI::HL::W::World &world, AI::HL::STP::Coordinate pos);

				/**
				 * The STP suggested method for passing.
				 * See instructions in passer_ready_position.
				 *
				 * \param [in] pos where the passer should be.
				 *
				 * \param [in] target where the passee should be.
				 */
				Tactic::Ptr passer_shoot(const AI::HL::W::World &world, AI::HL::STP::Coordinate pos, AI::HL::STP::Coordinate target);

				/**
				 * The STP suggested method for passing.
				 * See instructions in passer_ready_position.
				 *
				 * \param [in] pos where the passee should be.
				 */
				Tactic::Ptr passee_receive(const AI::HL::W::World &world, AI::HL::STP::Coordinate pos);
			}
		}
	}
}

#endif

