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
Tactic::Ptr move_once(
    World world, Point dest);  // move once to a location specified by dest
Tactic::Ptr move_once(
    World world, Point dest, Angle orientation);  // move once to a location
                                                  // specified by dest, with the
                                                  // specified final orientation

Tactic::Ptr move(
    World world, Point dest);  // move to a location specified by dest
Tactic::Ptr move(World world, Point dest, Angle orientation);  // move to a
                                                               // location
                                                               // specified by
                                                               // dest with the
                                                               // specified
                                                               // final
                                                               // orientation
}
}
}
}
