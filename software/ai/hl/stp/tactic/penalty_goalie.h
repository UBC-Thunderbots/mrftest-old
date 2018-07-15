#pragma once

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
 * Penalty Goalie
 * Active Tactic (But it's never done)
 * Only to be used for defending against penalty kicks.
 */
Tactic::Ptr penalty_goalie(World world);
}
}
}
}
