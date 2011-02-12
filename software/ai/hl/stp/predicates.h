#ifndef AI_HL_STP_PREDICATES
#define AI_HL_STP_PREDICATES

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * A predicate is a function object that returns a boolean.
			 * It is used for choosing a play.
			 *
			 * IMPORTANT!
			 * Predicates must follow the flyweight design pattern.
			 * There is only one instance of each unique (predicate,parameter) pair.
			 *
			 * Predicates are objects so they can be stored in some container.
			 */
			class Predicate {
				public:
					virtual bool evaluate(AI::HL::W::World& world) const = 0;
			};

			const Predicate* playtype(const AI::HL::W::PlayType::PlayType playtype);

			const Predicate* our_ball();

			const Predicate* their_ball();

			const Predicate* our_team_size(const unsigned int n);

			const Predicate* their_team_size(const unsigned int n);

			const Predicate* ball_x_less_than(const double x);

			const Predicate* ball_x_greater_than(const double x);

			const Predicate* ball_on_our_side();

			const Predicate* ball_on_their_side();
		}
	}
}

#endif

