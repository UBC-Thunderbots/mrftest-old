#ifndef AI_HL_STP_PREDICATES
#define AI_HL_STP_PREDICATES

#include "ai/hl/stp/world.h"
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
				bool goal(const World &);

				bool playtype(const World &world, AI::Common::PlayType playtype);

				bool our_ball(const World &world);

				bool their_ball(const World &world);

				bool none_ball(const World &world);

				bool our_team_size_at_least(const World &world, const unsigned int n);

				bool their_team_size_at_least(const World &world, const unsigned int n);

				bool their_team_size_at_most(const World &world, const unsigned int n);

				bool ball_x_less_than(const World &world, const double x);

				bool ball_x_greater_than(const World &world, const double x);

				bool ball_on_our_side(const World &world);

				bool ball_on_their_side(const World &world);
				
				bool ball_in_our_corner(const World &world);
				
				bool ball_in_their_corner(const World &world);
				
				bool ball_midfield(const World &world);
				
				bool baller_can_shoot(const World &world);
			}
		}
	}
}

#endif

