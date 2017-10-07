#pragma once

#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/world.h"
#include "util/dprint.h"

using namespace AI::HL::W;

namespace AI
{
namespace HL
{
namespace STP
{
namespace Action
{
/**
 * Move
 *
 * Move to a particular location and stop. Orient the player
 * towards a particular direction.
 */
void move(caller_t& ca, World world, Player player, Point dest);

void move(
    caller_t& ca, World world, Player player, Point dest, Angle orientation);
void move_straight(caller_t& ca, World world, Player player, Point dest);

void move_straight(
    caller_t& ca, World world, Player player, Point dest, Angle orientation);

void move_rrt(caller_t& ca, World world, Player player, Point dest);

void move_rrt(
    caller_t& ca, World world, Player player, Point dest, Angle orientation);

void move_slp(caller_t& ca, World world, Player player, Point dest);

void move_slp(
    caller_t& ca, World world, Player player, Point dest, Angle orientation);

/**
                 * Move
                 *
                 *
                 * Move to a particular location and stop. Orient the player
                 * towards a particular direction.
                 */
void move_dribble(
    caller_t& ca, World world, Player player, Angle orientation, Point dest);

/**
 * Move
 *
 * Move to a particular location with a low velocity and stop.
 * Orient the player towards the ball. MOVE_CAREFUL flag is set.
 */
void move_careful(caller_t& ca, World world, Player player, Point dest);

/**
 * wait function with the condition: player gets with tolerence radius around
 * the ball
 */
inline void wait_move(caller_t& ca, Player player, Point dest)
{
    // double tolerance = Robot::MAX_RADIUS + 0.005;
    double tolerance = Robot::MAX_RADIUS + 0.015;

    // LOG_INFO(u8"in wait move before yield");
    while ((player.position() - dest).len() > tolerance)
    {
        LOGF_INFO(
            u8"in wait move: Current position:%1, Dest:%2", player.position(),
            dest);
        LOGF_INFO(
            u8"in wait move: dist to dest: %1, tol: %2",
            (player.position() - dest).len(), tolerance);
        Action::yield(ca);
    }
    LOG_INFO(u8"in wait move after done");
}

/**
 *
 */
inline void wait_move(
    caller_t& ca, Player player, Point dest, Angle final_orient)
{
    double tolerance       = Robot::MAX_RADIUS + 0.015;
    double angle_tolerance = 0.5;  // in degrees

    while ((player.position() - dest).len() > tolerance ||
           player.orientation().to_degrees() >
               final_orient.to_degrees() + angle_tolerance ||
           player.orientation().to_degrees() <
               final_orient.to_degrees() - angle_tolerance)
    {
        Action::yield(ca);
    }
}
}
}
}
}
