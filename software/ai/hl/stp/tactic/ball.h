#pragma once

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/region.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Tactic {

				/**
				 * Spin steal
				 * Active Tactic
				 * Move spin up to the enemy with the ball and
				 * attempt to steal the ball
				 */
				Tactic::Ptr spin_steal(World world);

				/**
				 * Back up to steal the ball
				 * Active Tactic
				 * Go up to the ball and attempt to steal, once the ball has
				 * been stolen backup a distance of 4 * the robot radius
				 */
				Tactic::Ptr back_up_steal(World world);

				/**
				 * Active Defense
				 * Active Tactic
				 * Find the enemy robot that is in possession of the ball, move
				 * spin up to them and attempt to take the ball and take it
				 * as far as possible away from our goal
				 */
				Tactic::Ptr tactive_def(World world);

				/**
				 * Dribble to Region
				 * Active Tactic
				 * Use the dribble action to dribble the ball to the center
				 * of the region
				 */
				Tactic::Ptr tdribble_to_region(World world, Region _region);

				/**
				 * Spin to Region
				 * Active Tactic
				 * Use the spin to region action to move to the center of
				 * the region
				 */
				Tactic::Ptr tspin_to_region(World world, Region _region);
			}
		}
	}
}
