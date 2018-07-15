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
 * Invokes the catch primitive
 */
void just_catch_ball(caller_t& ca, World world, Player player);

/**
 * Navigate to the catch position then catch the ball
 */

void catch_ball(caller_t& ca, World world, Player player);

}
}
}
}
