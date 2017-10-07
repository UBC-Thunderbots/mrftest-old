#pragma once

#include "ai/hl/stp/action/action.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Action
{
/**
 * Attempts to catch / intercept the ball. If the ball is moving slow enough
 * the catcher will face the target, otherwise it will face the ball.
 * If no target is given it will assume the target is the enemy goal
 */
void just_catch_ball(caller_t& ca, World world, Player player);
void catch_ball(
    caller_t& ca, World world, Player player, Point target = Point(-99, -99));
}
}
}
}
