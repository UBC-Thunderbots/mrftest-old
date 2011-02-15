#ifndef AI_HL_STP_EVALUATION_DEFENSE_H
#define AI_HL_STP_EVALUATION_DEFENSE_H

#include "ai/hl/world.h"
#include "ai/hl/stp/evaluation/evaluation.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * A generic defense class, for 1 goalie and 1-2 defenders.
				 * A play should have an instance of a subclass.
				 */
				class Defense : public Module {
					public:
						Defense(AI::HL::STP::Play::Play &play) : Module(play) {
						}

						/**
						 * Best location for goalie.
						 */
						virtual Point goalie_dest() const = 0;

						/**
						 * Best location for defender1.
						 */
						virtual Point defender1_dest() const = 0;

						/**
						 * Best location for defender2.
						 */
						virtual Point defender2_dest() const = 0;
				};

				/**
				 * A cone defense mechanism used in Singapore Robocup 2010.
				 * Must have at least 1 defender.
				 */
				class ConeDefense : public Defense {
					public:
						ConeDefense(AI::HL::STP::Play::Play &play, AI::HL::W::World &world);

						void evaluate();

						Point goalie_dest() const {
							return goalie_dest_;
						}

						Point defender1_dest() const {
							return defender1_dest_;
						}

						Point defender2_dest() const {
							return defender2_dest_;
						}

					protected:
						AI::HL::W::World &world;

						bool goalie_top;

						Point goalie_dest_;
						Point defender1_dest_;
						Point defender2_dest_;
				};
			}
		}
	}
}

#endif

