#ifndef AI_HL_STP_PREDICATES
#define AI_HL_STP_PREDICATES

#include "ai/hl/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Predicates {
				/**
				 * A predicate is a condition object that returns a boolean.
				 * It is used for choosing a play.
				 *
				 * IMPORTANT!
				 * Predicates must follow the flyweight design pattern.
				 * There is only one instance of each unique (predicate,parameter) pair.
				 * Having pointers to singletons allows natural comparison and equivalence operation.
				 *
				 * Because predicates are singletons,
				 * they should NOT reference a WORLD object and are IMMUTABLE.
				 *
				 * Predicates are pointers so they can be stored in some container.
				 * Perhaps in the future, we can generate a decision tree based on the predicates.
				 *
				 * TODO:
				 * This is not the best implementation.
				 * Someone should investigate something cooler,
				 * like tr1::function,
				 * std::function<bool (AI::HL::W::World&)> Predicate;
				 */
				class Predicate {
					public:
						virtual bool evaluate(AI::HL::W::World& world) const = 0;
				};

				const Predicate* negate(const Predicate* predicate);

				const Predicate* goal();

				const Predicate* playtype(const AI::HL::W::PlayType::PlayType playtype);

				const Predicate* our_ball();

				const Predicate* their_ball();

				const Predicate* none_ball();

				const Predicate* our_team_size_at_least(const unsigned int n);

				const Predicate* their_team_size_at_least(const unsigned int n);

				const Predicate* their_team_size_at_most(const unsigned int n);

				const Predicate* ball_x_less_than(const double x);

				const Predicate* ball_x_greater_than(const double x);

				const Predicate* ball_on_our_side();

				const Predicate* ball_on_their_side();

				/**
				 * Evaluates all predicates in this container.
				 */
				template<class Container> bool evaluate(AI::HL::W::World& world, const Container& predicates) {
					for (typename Container::const_iterator it = predicates.begin(); it != predicates.end(); ++it) {
						if (!(*it)->evaluate(world)) {
							return false;
						}
					}
					return true;
				}

			}
		}
	}
}

#endif

