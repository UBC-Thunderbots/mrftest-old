#pragma once

#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/enemy.h"
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
 * Shadow Kickoff
 * Not active tactic
 * Shadow a specific enemy robot on the enemy kickoff.
 */
Tactic::Ptr shadow_kickoff(
    World world, Enemy::Ptr enemy, const Coordinate default_loc);

/**
 * Shadow Ball
 * Not active tactic
 * Shadow the ball (in freekicks).
 */
Tactic::Ptr shadow_ball(World world);
}
}
}
}
