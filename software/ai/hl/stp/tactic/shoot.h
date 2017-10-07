#pragma once

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/tactic.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Tactic
{
/**
 * Shoot Goal
 * Shoot for the enemy goal.
 */
Tactic::Ptr shoot_goal(World world);

/**
 * Shoot Target
 * Shoot a specified target.
 */
Tactic::Ptr shoot_target(World world, const Coordinate target);
}
}
}
}
