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
 * Shoot Goal
 *
 * Not intended for goalie use
 *
 * Shoots the ball at the largest open angle of the enemy goal.
 */
void shoot_goal(caller_t& ca, World world, Player player);

/**
 * Shoot Target
 *
 * Not intended for goalie use
 *
 */
void shoot_target(
    caller_t& ca, World world, Player player, const Point target,
    double velocity = BALL_MAX_SPEED, bool chip = false);

/**
 * Shoot Target
 *
 * Not intended for goalie use
 *
 */
void catch_and_shoot_target(
    caller_t& ca, World world, Player player, const Point target,
    double velocity = BALL_MAX_SPEED, bool chip = false);

void catch_and_shoot_goal(caller_t& ca, World world, Player player);
}
}
}
}
