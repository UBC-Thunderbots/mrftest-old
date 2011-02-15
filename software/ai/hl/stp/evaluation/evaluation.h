#ifndef AI_HL_STP_EVALUATION_EVALUATION
#define AI_HL_STP_EVALUATION_EVALUATION

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
						Module(AI::HL::STP::Play::Play &p);

						/**
						 * Destructor.
						 */
						~Module();

					private:
						AI::HL::STP::Play::Play &play;
						sigc::connection connection;

						/**
						 * Runs the associated computation every tick.
						 * Only a play can run this.
						 */
						virtual void evaluate() = 0;

						friend class AI::HL::STP::Play::Play;
				};
			}
		}
	}
}

#endif

