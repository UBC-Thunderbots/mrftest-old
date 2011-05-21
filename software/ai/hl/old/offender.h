#ifndef AI_HL_OLD_ROLE_OFFENDER_H
#define AI_HL_OLD_ROLE_OFFENDER_H

#include "ai/hl/world.h"
#include <vector>

namespace AI {
	namespace HL {
		/**
		 * Finds weak positions on the enemy goal area
		 * and computes positions on the field so that players can shoot the enemy goal.
		 *
		 * In other words, finds location whereby if u pass the ball to these players,
		 * they can shoot to the enemy goal easily.
		 *
		 * Hence this role is useful in finding opportunities to get goals.
		 *
		 * By default, attempts to chase after the ball.
		 */
		class Offender {
			public:
				/**
				 * Constructs a new Offender.
				 *
				 * \param[in] w the world.
				 */
				Offender(W::World &w);

				/**
				 * Toggles whether this role should chase the ball.
				 *
				 * \param[in] \c true to chase the ball, or \c false to not.
				 */
				void set_chase(bool b) {
					chase = b;
				}

				/**
				 * Sets the players.
				 *
				 * \param[in] p the players who should go on offense.
				 */
				void set_players(const std::vector<W::Player::Ptr> &p) {
					players = p;
				}

				/**
				 * Runs the offenders for one tick.
				 */
				void tick();

			protected:
				/**
				 * The scoring function for having the player in the particular position.
				 *
				 * \param [in] dont_block list of points which should have sight to the goal area, which should include the ball, chasers, etc.
				 */
				double scoring_function(const std::vector<Point> &enemy_pos, const Point &pos, const std::vector<Point> &dont_block) const;

				/**
				 * Assume that role has the ball.
				 * Find where to position the players,
				 * so that it has the greatest chance of shooting.
				 * The enemy position is provided as vector,
				 * so we can add imaginary enemies.
				 * If no position is valid,
				 * will simply choose the middle of the field.
				 */
				bool calc_position_best(const std::vector<Point> &enemy_pos, const std::vector<Point> &dont_block, Point &best_pos);

				/**
				 * The reason why you want to use this role.
				 * Calculates the best positions to place the players.
				 *
				 * \param[in] n the number of positions to calculate.
				 *
				 * \return the \p n best positions at which to place the players.
				 */
				std::vector<Point> calc_positions(const unsigned int n);

				/**
				 * The world.
				 */
				W::World &world;

				/**
				 * The players who are on offense.
				 */
				std::vector<W::Player::Ptr> players;

				/**
				 * \c true if the players should chase the ball, or \c false if not.
				 */
				bool chase;

				/**
				 * Stores which grids are possible to place players on.
				 */
				std::vector<std::vector<bool> > grid;
		};
	}
}

#endif

