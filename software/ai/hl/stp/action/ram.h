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
 * Ram
 *
 * Not intended for goalie use
 *
 * Move to a particular location and stop. Orient the player
 * towards the ball.
 */
void ram(caller_t& ca, World world, Player player, Point dest);

/**
 * Ram
 *
 * Not intended for goalie use
 *
 * Ram defaulting to ram the ball
 */
void ram(caller_t& ca, World world, Player player);

/**
 * Ram
 *
 * Intended for goalie use
 *
 * Move to a particular location and stop. Orient the player
 * towards the ball.
 */
void goalie_ram(caller_t& ca, World world, Player player, Point dest);
}
}
}
}
