#ifndef AI_HL_STP_EVALUATE_PASS_H
#define AI_HL_STP_EVALUATE_PASS_H

#include "ai/hl/world.h"
#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/evaluate/evaluate.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * Generic passing module.
				 */
				class Pass : public Module {
					public:
						Pass(AI::HL::STP::Play::Play& play) : Module(play) {
						}

						/**
						 * Best location for passer.
						 */
						virtual Point passer() const = 0;

						/**
						 * Best location for passee.
						 */
						virtual Point passee() const = 0;
				};

				/**
				 * Simple Pass using 2 coordinates.
				 */
				class SimplePass : public Pass {
					public:
						SimplePass(AI::HL::STP::Play::Play& play);

						void set_passer_pos(Coordinate p);

						void set_passee_pos(Coordinate p);

						void evaluate();

						Point passer() const;

						Point passee() const;

					protected:
						Coordinate passer_pos_;
						Coordinate passee_pos_;
				};
			}
		}
	}
}

#endif

