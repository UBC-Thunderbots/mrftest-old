#pragma once

#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/world.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Action
{
/**
 * Block Goal
 *
 * Not intended for goalie use
 *
 * Blocks against a single enemy from shooting to our goal.
 */
void block_goal(caller_t& ca, World world, Player player, Robot robot);

/**
 * Block Ball
 *
 * Not intended for goalie use
 *
 * Blocks against a single enemy from the ball / passing.
 */
void block_ball(caller_t& ca, World world, Player player, Robot robot);
}
}
}
}
