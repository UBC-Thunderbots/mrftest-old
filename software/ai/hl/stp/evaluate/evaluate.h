#ifndef AI_HL_STP_EVALUATE_EVALUATE
#define AI_HL_STP_EVALUATE_EVALUATE

#include "ai/hl/world.h"
#include "ai/hl/stp/play/play.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * An evaluation module is a class that does some heavy computation.
				 *
				 * IMPORTANT NOTE!
				 * Plays always take ownership of evaluation modules.
				 * Tactics or skills can only have references to modules,
				 * which are owned by plays.
				 */
				class Module : public NonCopyable {
					public:
						/**
						 * Constructor, must always reference a play.
						 */
						Module(AI::HL::STP::Play::Play& p);

						/**
						 * Runs the associated computation every tick.
						 */
						virtual void evaluate() = 0;

					protected:
						AI::HL::STP::Play::Play& play;
				};
			}
		}
	}
}

#endif

