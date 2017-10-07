#ifndef AI_HL_STP_TACTIC_UTIL_H
#define AI_HL_STP_TACTIC_UTIL_H

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
 * Selects a baller for active tactics.
 *
 * use a hysterisis
 */
Player select_baller(
    World world, const std::set<Player> &players, Player previous);
}
}
}
}

#endif
