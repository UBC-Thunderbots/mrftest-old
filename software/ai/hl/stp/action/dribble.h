#pragma once

#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/world.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Action
{
/**
 * Dribble
 *
 * Not intended for goalie use
 *
 * Dribble and stay at the same position.
 */
void dribble(caller_t& ca, Player player);

/**
 * Dribble
 *
 * Not intended for goalie use
 *
 * Dribble to a particular location and stop.
 */
void dribble(caller_t& ca, World world, Player player, Point dest);
void dribble(caller_t& ca, World world, Player player, Point dest, Angle ori);
void dribble(caller_t& ca, World world, Player player, Point dest, Point face);
}
}
}
}
