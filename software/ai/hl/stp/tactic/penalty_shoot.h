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
 * Penalty Shoot
 * Active Tactic
 * Shoot for the enemy goal with the shoot goal tactic.
 */
Tactic::Ptr penalty_shoot(AI::HL::W::World world);
}
}
}
}
