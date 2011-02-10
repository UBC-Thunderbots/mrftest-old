#ifndef AI_HL_STP_EVALUATION_OFFENSE_H
#define AI_HL_STP_EVALUATION_OFFENSE_H

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/evaluation/evaluation.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Calculates strategic positions to place 1-2 offensive players.
				 * Finds weak points on the enemy goal area,
				 * where the enemy net is open and the ball is visible.
				 */
				class Offense : public Module {
					public:
						Offense(AI::HL::STP::Play::Play& play) : Module(play) {
						}

						/**
						 * Best location for 1st offender.
						 */
						virtual Point offender1_dest() const = 0;

						/**
						 * Best location for 2nd offender.
						 */
						virtual Point offender2_dest() const = 0;
				};
			}
		}
	}
}

#endif

