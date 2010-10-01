#ifndef AI_HL_ROLE_OFFENDER_H
#define AI_HL_ROLE_OFFENDER_H

#include "ai/hl/world.h"
#include <vector>

namespace AI {
	namespace HL {

		/**
		 * Gets the players to go to their offensive positions.
		 * Tries to receive the ball if defender or goalie has it.
		 * If in possesion of ball, tries to find best positions to shoot and score.
		 */
		class Offender {
			public:
				Offender(W::World &w);

				/**
				 * Toggles whether this role should chase the ball.
				 */
				void set_chase(bool b) {
					chase = b;
				}

				/**
				 * Sets the players.
				 */
				void set_players(const std::vector<W::Player::Ptr> &p) {
					players = p;
				}

				void tick();

			protected:

				/**
				 * The scoring function for
				 * having the player in the particular position.
				 *
				 * \param [in] dont_block list of points which
				 * should have sight to the goal area.
				 * This should include the ball, and chasers etc.
				 */
				double scoring_function(const std::vector<Point>& enemy_pos, const Point& pos, const std::vector<Point>& dont_block) const;

				/**
				 * Assume that role has the ball.
				 * Find where to position the players,
				 * so that it has the greatest chance of shooting.
				 * The enemy position is provided as vector,
				 * so we can add imaginary enemies.
				 * If no position is valid,
				 * will simply choose the middle of the field.
				 */
				bool calc_position_best(const std::vector<Point>& enemy_pos, const std::vector<Point>& dont_block, Point& best_pos);

				/**
				 * The reason why you want to use this role.
				 * Calculates n best positions to place the players.
				 */
				std::vector<Point> calc_positions(const unsigned int n);

				W::World &world;

				std::vector<W::Player::Ptr> players;

				bool chase;

				/**
				 * Stores which grids are possible to place players on.
				 */
				std::vector<std::vector<bool> > grid;

		};
	}
}

#endif

