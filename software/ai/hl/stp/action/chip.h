#pragma once

#include "ai/hl/stp/world.h"
#include "ai/hl/stp/action/action.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				/**
				 * Chip Target
				 *
				 * Not intended for goalie use
				 *
				 * Chips the ball to a target point with a double param power
				 * indicating the power to chip.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool chip_target(caller_t& ca, World world, Player player, const Point target);

				/**
				 * Goalie Chip Target
				 *
				 * Intended for goalie use
				 * 
				 * Chips the ball to a target point with a double param power
				 * indicating the power to chip.
				 *
				 * \return true if the player autokick is fired.
				 */
				bool goalie_chip_target(caller_t& ca, World world, Player player, const Point target);
				
				/*
				 * Chip at Location
				 *
				 * Not intended for goalie use
				 *
				 * Goes to a location on the field and chips the ball at a certain orientation.
				 */
				bool chip_at_location(caller_t& ca, World world, Player player, const Point location_to_chip_at,
					double chip_distance, Angle chip_orientation);
			}
		}
	}
}
