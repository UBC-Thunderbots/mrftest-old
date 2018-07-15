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

DoubleParam EDGE_CATCH_DIST(u8"Edge catch distance", "AI/HL/STP/Action/catch", 0.2, 0.05, 0.5);
DoubleParam CATCH_PRIM_ACTIVATION_DIST
        (u8"Catch Primitive activation distance", "AI/HL/STP/Action/catch", 0.3, 0.0, 1.0);
DoubleParam OVERSHOOT_FACTOR
        (u8"Factor to overestimate the intercept time by", "AI/HL/STP/Action/catch", 1.2, 1.0, 1.5);

void AI::HL::STP::Action::just_catch_ball(
        caller_t& ca, World world, Player player)
{
    AI::BE::Primitives::Ptr prim(
            new Primitives::Catch(player, world.ball().velocity().len(), 8000, 0.1));

    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
}


void AI::HL::STP::Action::catch_ball(
        caller_t& ca, World world, Player player)
{

    if ((world.ball().position() - player.position()).len() < CATCH_PRIM_ACTIVATION_DIST)
    {
        Action::just_catch_ball(ca, world, player);
    }

    // solve for the intercept point
    Point intercept_point(99,99);
    Angle intercept_angle = (-world.ball().velocity()).orientation();
    for (double delta_t = 0.0; delta_t < 2.0; delta_t += 0.1)
    {
        Point ball_projected_position = world.ball().position(delta_t * OVERSHOOT_FACTOR);

        // intercept along the long sides if the ball gets too close

        if (std::fabs(ball_projected_position.y - (world.field().width() / 2)) <= EDGE_CATCH_DIST)
        {
            auto field_line_seg = Geom::Seg(Point(world.field().length() / 2, world.field().width() / 2),
                                            Point(-world.field().length() / 2, world.field().width() / 2));

            intercept_point = line_intersect(field_line_seg.start,
                                             field_line_seg.end,
                                             world.ball().position(),
                                             ball_projected_position + world.ball().velocity(delta_t) * 10);
        }

        if (std::fabs(ball_projected_position.y - (-world.field().width() / 2)) <= EDGE_CATCH_DIST)
        {
            auto field_line_seg = Geom::Seg(Point(world.field().length() / 2, -world.field().width() / 2),
                                            Point(-world.field().length() / 2, -world.field().width() / 2));

            intercept_point = line_intersect(field_line_seg.start,
                                             field_line_seg.end,
                                             world.ball().position(),
                                             ball_projected_position + world.ball().velocity(delta_t) * 10);
        }


        if ((player.position() - ball_projected_position).len() < (player.position() - intercept_point).len())
        {
            intercept_point = ball_projected_position;
        }

        intercept_angle = - world.ball().velocity(delta_t).orientation();
    }

    AI::HL::STP::Action::move(ca, world, player, intercept_point, intercept_angle);
}
