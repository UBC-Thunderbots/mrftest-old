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
 * Move to a location specified by dest.
 *
 * \param[in] dest the location to move to.
 */
Tactic::Ptr move_once(World world, Point dest);

/**
 * Move, and follow, a location specified by dest.
 *
 * \param[in] dest the location to move to.
 */
Tactic::Ptr move(World world, Coordinate dest);
}
}
}
}
