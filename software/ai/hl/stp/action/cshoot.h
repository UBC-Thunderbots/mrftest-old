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
 * Circular Shoot Target
 *
 * Not intended for goalie use
 *
 * Shoots the ball to a target point with a double param kicking
 * speed for passing.
 *
 * \param[in] pass true if the player is to pass.
 *
 * \return true if the player autokick is fired.
 */
bool cshoot_target(
    caller_t& ca, World world, Player player, const Point target,
    double velocity = BALL_MAX_SPEED);
}
}
}
}
