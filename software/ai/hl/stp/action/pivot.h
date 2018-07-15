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
 * Pivot around the ball until the player is oriented in the direction of the
 * target.
 *
 *  \param[in] target Used to determine the direction the player should be
 * facing when player finishes pivoting.
 */
void pivot(
    caller_t& ca, World world, Player player, Point target, Angle finalAngle,
    double radius = 0);
}
}
}
}
