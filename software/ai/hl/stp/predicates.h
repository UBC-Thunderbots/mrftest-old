#ifndef AI_HL_STP_PREDICATES
#define AI_HL_STP_PREDICATES

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/region.h"
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

				bool our_team_size_exactly(const World &world, const unsigned int n);

				bool their_team_size_at_least(const World &world, const unsigned int n);

				bool their_team_size_at_most(const World &world, const unsigned int n);

				bool ball_x_less_than(const World &world, const double x);

				bool ball_x_greater_than(const World &world, const double x);

				bool ball_on_our_side(const World &world);

				bool ball_on_their_side(const World &world);

				bool ball_in_our_corner(const World &world);

				bool ball_in_their_corner(const World &world);

				bool ball_midfield(const World &world);

				bool ball_near_friendly_goal(const World &world);
				
				bool ball_near_enemy_goal(const World &world);

				/**
				 * player with the ball can shoot at their goal
				 */
				bool baller_can_shoot(const World &world);

				/**
				 * player with the ball can pass
				 */
				bool baller_can_pass(const World &world);

				/**
				 * player with the ball can shoot at a target point
				 */
				bool baller_can_pass_target(const World &world, const Point target);

				/**
				 * player with the ball is under threat (surrounded by enemies)
				 */
				bool baller_under_threat(const World &world);

				/**
				 * enemy with the ball can shoot at our goal
				 */
				bool enemy_baller_can_shoot(const World &world);

				/**
				 * enemy with the ball can pass to another enemy
				 */
				bool enemy_baller_can_pass(const World &world);

				/**
				 * # of passes from the enemy with ball is > 0 (no clear shot)
				 * and < 3 (irrelevant to be any higher)
				 * to be able to get a clear shoot at our goal
				 */
				bool enemy_baller_can_pass_shoot(const World &world);

				/**
				 * borrowed from cm, true if our ball or ball on their side
				 */
				bool offensive(const World &world);

				/**
				 * borrowed from cm, true if their ball or ball on our side
				 */
				bool defensive(const World &world);

				/**
				 * borrowed from cm, true if number of enemies on our side is greater than n
				 */
				bool num_of_enemies_on_our_side_at_least(const World &world, const unsigned int n);

				/**
				 * true if ball is inside region
				 */
				bool ball_inside_region(const World &world, Region region);

				/**
				 * true if our_ball and their_ball (so we have to fight for the ball)
				 */
				bool fight_ball(const World &world);				

				bool can_shoot_ray(const World& world);
				
				/**
				 * true if the ball is inside some robot
				 */
				bool ball_inside_robot(const World &world);
			}
		}
	}
}

#endif

