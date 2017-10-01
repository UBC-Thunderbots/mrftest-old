#ifndef AI_HL_STP_TACTIC_TDEFEND_H
#define AI_HL_STP_TACTIC_TDEFEND_H

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
 *
 * Layered defense by Terence. Must have one of tgoalie or tdefender1
 */

/**
 * TGoalie
 * Not Active Tactic
 * A tactic for Terence goalie. Guards defense area (and some portion of the
 * inner layer).
 */
Tactic::Ptr tgoalie(World world, const size_t defender_role);

/**
 * TDefender1
 * Not Active Tactic
 * A tactic for Terence defender 1. Guards defense area, inner layer, and outer
 * layer.
 */
Tactic::Ptr tdefender1(World world, bool active_baller = false);

/**
 * TDefender2
 * Not Active Tactic
 * A tactic for Terence defender 2. Guards inner layer, outer layer, and our
 * side of the field.
 */
Tactic::Ptr tdefender2(World world, bool active_baller = false);

/**
 * TDefender3
 * Not Active Tactic
 * A tactic for Terence defender 3. Guards same layer as tdefender1.
 */
Tactic::Ptr tdefender3(World world, bool active_baller = false);

/**
 * TDefend Line
 * Not Active Tactic
 * Defend a line
 * If p1_ == p2_ it'll be equivalent as defending a point
 */
Tactic::Ptr tdefend_line(
    World world, Coordinate p1_, Coordinate p2_, double dist_min_,
    double dist_max_);
}
}
}
}

#endif
