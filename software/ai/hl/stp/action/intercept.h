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
 * Intercept
 *
 * Not intended for goalie use
 *
 * Intercept the ball facing towards the target.
 *
 *  \param[in] target Used to determine the direction the player should be
 * facing when player intercepts the ball.
 */
void intercept(caller_t& ca, World world, Player player, const Point target);
}
}
}
}
