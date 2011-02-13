#ifndef AI_HL_STP_PREDICATES
#define AI_HL_STP_PREDICATES

#include "ai/hl/world.h"
#include <functional>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * A predicate is a function that returns true or false
			 * depending on the world condition.
			 * A play is more READABLE if its conditions are written as functions.
			 */
			namespace Predicates {
				bool goal(AI::HL::W::World&);

				bool playtype(AI::HL::W::World& world, const AI::HL::W::PlayType::PlayType playtype);

				bool our_ball(AI::HL::W::World& world);

				bool their_ball(AI::HL::W::World& world);

				bool none_ball(AI::HL::W::World& world);

				bool our_team_size_at_least(AI::HL::W::World& world, const unsigned int n);

				bool their_team_size_at_least(AI::HL::W::World& world, const unsigned int n);

				bool their_team_size_at_most(AI::HL::W::World& world, const unsigned int n);

				bool ball_x_less_than(AI::HL::W::World& world, const double x);

				bool ball_x_greater_than(AI::HL::W::World& world, const double x);

				bool ball_on_our_side(AI::HL::W::World& world);

				bool ball_on_their_side(AI::HL::W::World& world);
			}
		}
	}
}

#endif

