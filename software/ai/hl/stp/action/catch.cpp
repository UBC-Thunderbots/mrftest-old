#include "ai/hl/stp/action/catch.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/intercept.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;

const double closeToTargetOrientation = 20;  // when the robot is this many
                                             // degrees off of it's target axis
                                             // ignore ball distance flags
const double transitioningToTargetOrientation =
    30;  // when the robot is this many degrees off of it's target axis, avoid
         // the ball by a small margin
const double stoppedBallTolerence =
    0.1;  // if the ball is movign slower than this, assume it is not moving
const double catchBallScaleFactor =
    1.3;  // used to scale the radius of the robot. Used to determine a safe
          // distance from the ball to "catch it"
const double distanceCatchCondition =
    0.01;  // used to determine if the robot is close close enough to the safe
           // catch position to have officially "caught" it
const double velocityCatchCondition = 0.7 * 0.7;  // if the robot is moving
                                                  // faster than this, the ball
                                                  // cannot be "caught"
const double angleCatchCondition = 8;  // if the robot's orientation is off my
                                       // more than this many degrees, the ball
                                       // cannot be "caught"

void AI::HL::STP::Action::just_catch_ball(
    caller_t& ca, World world, Player player)
{
    player.send_prim(Drive::move_catch(Angle(), 0, 0));
    while (!player.has_ball())
    {
        Action::yield(ca);
    }
}

void AI::HL::STP::Action::catch_ball(
    caller_t& ca, World world, Player player, Point target)
{
    Point target_line = world.ball().position() -
                        target;  // the line between the ball and target
    Point catch_pos;  // prediction of where the robot should be behind the ball
                      // to catch it
    Angle catch_orientation;  // The orientation the catcher should have when
                              // catching

    do
    {
        // The angle between the target line, and the line from the robot to the
        // ball
        Angle orientation_diff = (player.position() - world.ball().position())
                                     .orientation()
                                     .angle_diff(target_line.orientation());

        // sets the avoid ball flags based on how close to robot is to the ball
        // (and if it's on the correct side)
        if (orientation_diff < Angle::of_degrees(closeToTargetOrientation))
        {
            player.flags(AI::Flags::calc_flags(world.playtype()));
        }
        else if (
            orientation_diff <
            Angle::of_degrees(transitioningToTargetOrientation))
        {
            player.flags(
                AI::Flags::calc_flags(world.playtype()) |
                AI::Flags::MoveFlags::AVOID_BALL_TINY);
        }
        else
        {
            player.flags(
                AI::Flags::calc_flags(world.playtype()) |
                AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
        }

        target_line = world.ball().position() - target;

        if (world.ball().velocity().lensq() < stoppedBallTolerence)
        {
            LOG_INFO("SITTING STILL");
            // ball is either very slow or stopped. prediction algorithms don't
            // work as well so
            // just go behind the ball facing the target
            catch_pos =
                world.ball().position() +
                target_line.norm(Robot::MAX_RADIUS * catchBallScaleFactor);
            catch_orientation =
                (target - world.ball().position()).orientation();
        }
        else
        {
            LOG_INFO("MOVING BALL");
            catch_pos = Evaluation::calc_fastest_grab_ball_dest(world, player);
            catch_orientation =
                (world.ball().position() - catch_pos).orientation();
        }

        Action::move(ca, world, player, catch_pos, catch_orientation);

        Action::yield(ca);
    } while ((player.position() - catch_pos).lensq() > distanceCatchCondition ||
             player.velocity().lensq() > velocityCatchCondition ||
             player.orientation().angle_diff(catch_orientation).abs() >
                 Angle::of_degrees(angleCatchCondition));
}
