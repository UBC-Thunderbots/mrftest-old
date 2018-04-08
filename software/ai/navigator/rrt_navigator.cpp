#include "ai/navigator/rrt_navigator.h"
#include <iostream>
#include "ai/hl/stp/param.h"
#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_planner.h"
#include "ai/navigator/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;
using AI::BE::Primitives::Primitive;
using AI::BE::Primitives::PrimitiveDescriptor;

namespace AI
{
namespace Nav
{
namespace RRT
{
IntParam jon_hysteris_hack(u8"Jon Hysteris Hack", u8"AI/Nav/RRT", 2, 1, 10);

DoubleParam angle_increment(
    u8"angle increment (deg)", u8"AI/Nav/RRT", 10, 1, 100);
DoubleParam linear_increment(
    u8"linear increment (m)", u8"AI/Nav/RRT", 0.05, 0.001, 1);

DoubleParam default_desired_rpm(
    u8"The default desired rpm for dribbling", u8"AI/Movement/Primitives", 7000,
    0, 100000);

class RRTNavigator final : public Navigator
{
   public:
    explicit RRTNavigator(World world);
    void tick() override;
    void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) override;
    NavigatorFactory &factory() const override;

   private:
    void plan(Player player);

    enum ShootActionType
    {
        NO_ACTION_OR_PIVOT = 0,
        SHOULD_SHOOT,
        SHOULD_MOVE,
        SHOULD_PIVOT,
        NO_ACTION_OR_MOVE,
        SHOOT_FAILED
    };
    RRTPlanner planner;
};
}
}
}

using AI::Nav::RRT::RRTNavigator;

RRTNavigator::RRTNavigator(AI::Nav::W::World world)
    : Navigator(world), planner(world)
{
}

void RRTNavigator::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx)
{
    ctx->set_source_rgb(1.0, 1.0, 1.0);

    for (const Player player : world.friendly_team())
    {
        if (!std::dynamic_pointer_cast<PlayerData>(
                player.object_store()[typeid(*this)]))
        {
            player.object_store()[typeid(*this)] =
                std::make_shared<PlayerData>();
        }

        PlayerData::Ptr player_data = std::dynamic_pointer_cast<PlayerData>(
            player.object_store()[typeid(*this)]);

        if (has_destination(player_data->hl_request))
        {
            Point dest  = player_data->hl_request.field_point();
            Angle angle = player_data->hl_request.field_angle();
            ctx->set_source_rgb(1, 1, 1);
            ctx->begin_new_path();
            ctx->arc(
                dest.x, dest.y, 0.09, angle.to_radians() + M_PI_4,
                angle.to_radians() - M_PI_4);
            ctx->stroke();
        }
    }
}

void RRTNavigator::plan(Player player)
{
    if (!std::dynamic_pointer_cast<PlayerData>(
            player.object_store()[typeid(*this)]))
    {
        player.object_store()[typeid(*this)] = std::make_shared<PlayerData>();
    }

    PlayerData::Ptr player_data = std::dynamic_pointer_cast<PlayerData>(
        player.object_store()[typeid(*this)]);

    PrimitiveDescriptor hl_request(Drive::Primitive::STOP, 0, 0, 0, 0, 1);

    if (player.has_prim())
    {
        hl_request = player.top_prim()->desc();
        player.top_prim()->active(true);
    }

    // just a hack for now, defense logic should be implemented somewhere else
    // positive x is enemy goal
    double x_limit =
        world.field().enemy_goal().x - world.field().defense_area_width() / 2;
    double y_limit = (world.field().defense_area_stretch() +
                      world.field().defense_area_width() * 2) /
                     2;

    bool defense_area_violation =
        hl_request.field_point().x > x_limit &&
        std::fabs(hl_request.field_point().y) < y_limit &&
        ((player.flags() & AI::Flags::MoveFlags::AVOID_ENEMY_DEFENSE) !=
         AI::Flags::MoveFlags::NONE);

    // starting a primitive
    PrimitiveDescriptor nav_request = hl_request;

    PrimitiveDescriptor nav_dest    = hl_request;

    std::vector<Point> plan;

    switch (hl_request.prim)
    {
        case Drive::Primitive::STOP:
            // No planning.
            break;
        case Drive::Primitive::MOVE:
        case Drive::Primitive::DRIBBLE:
        case Drive::Primitive::SHOOT:
        case Drive::Primitive::SPIN:
            // These all try to move to a target position. If we can’t
            // get there, do an RRT plan and MOVE to the next path
            // point instead.
            if (!valid_path(
                    player.position(), hl_request.field_point(), world, player))
            {
#warning Do we need flags here, e.g. to let the goalie into the defense area?
                plan = planner.plan(
                    player, hl_request.field_point(),
                    AI::Flags::MoveFlags::NONE);

                if (!plan.empty())
                {
                    nav_request.params[0] = plan[0].x;
                    nav_request.params[1] = plan[0].y;

                    nav_dest.params[0] = plan.back().x;
                    nav_dest.params[1] = plan.back().y;
                }

                player.display_path(plan);
            }
            break;

        case Drive::Primitive::CATCH:
#warning Check how to do clear path checking and RRT planning during catch.
            break;
        case Drive::Primitive::PIVOT:
#warning Check how to do clear path checking and RRT planning during pivot.
            break;
        case Drive::Primitive::DIRECT_WHEELS:
        case Drive::Primitive::DIRECT_VELOCITY:
            // The AI should not use these.
            std::abort();
    }

    double hl_request_len  = hl_request.field_point().len();
    double nav_request_len = nav_request.field_point().len();

    /*
    TODO do done stuff
    if (is_done(player, nav_dest) && player.has_prim()) {
            // stop primitive and continue with next one
            player.top_prim()->active(false);
            player.top_prim()->done(true);

            return;
    }
    */

    player_data->hl_request  = hl_request;
    player_data->nav_request = nav_request;

    // execute the plan
    // some calculation that are generally useful
    Point pos_diff = nav_request.field_point() - player.position();

    PrimitiveDescriptor pivot_request;
    PrimitiveDescriptor pivot_local;
    PrimitiveDescriptor shoot_request;
    Point target_position =
        Point::of_angle(hl_request.field_angle()) + hl_request.field_point();

    double final_velocity = 0.0F;

    switch (nav_request.prim)
    {
        case Drive::Primitive::STOP:
            if (nav_request.extra & 1)
            {
                player.move_brake();
            }
            else
            {
                player.move_coast();
            }
            break;
        case Drive::Primitive::MOVE:
            if (nav_request.extra & 1)
                nav_request.field_angle() =
                    Angle();  // fill in angle so the function doesn't crash
            LOG_DEBUG(Glib::ustring::compose(
                "Time for new move, point %1", nav_request.field_point()));

            if (plan.size() >= 2)
            {
                final_velocity = AI::Nav::Util::get_final_velocity(
                    player.position(), plan[0], plan[1]);
            }

            player.move_move(
                nav_request.field_point(), nav_request.field_angle(),
                final_velocity);

            break;
        case Drive::Primitive::DRIBBLE:
            LOG_DEBUG(Glib::ustring::compose(
                "Time for new dribble point %1, angle %2",
                nav_request.field_point(), nav_request.field_angle()));
            player.move_dribble(
                nav_request.field_point(), nav_request.field_angle(),
                default_desired_rpm, false);
            break;
        case Drive::Primitive::SHOOT:
            // deleted all of the old conditional code- too hacky, need to
            // rewrite
            // this probably doesn't currently work
            player.move_shoot(
                nav_request.field_point(), nav_request.params[3],
                nav_request.extra & 1);
            break;
        case Drive::Primitive::CATCH:
            player.move_catch(
                Angle::of_radians(nav_request.params[0]), nav_request.params[1],
                nav_request.params[2]);
            break;
        case Drive::Primitive::PIVOT:
            LOG_DEBUG(Glib::ustring::compose(
                "time for new pivot (pos %1, angle %2)",
                hl_request.field_point(), hl_request.field_angle()));
            player.move_pivot(
                hl_request.field_point(), hl_request.field_angle(),
                hl_request.field_angle_2());
            break;
        case Drive::Primitive::SPIN:
            LOG_DEBUG(Glib::ustring::compose(
                "Time for new spin, point %1, angle speed %2",
                nav_request.field_point(), hl_request.field_angle()));
            player.move_spin(
                nav_request.field_point(), hl_request.field_angle());
            break;
        default:
            LOG_ERROR(Glib::ustring::compose(
                "Unhandled primitive! (%1)",
                static_cast<int>(nav_request.prim)));
            break;
    }
    // TODO test if primitive is done
}

void RRTNavigator::tick()
{
    for (Player player : world.friendly_team())
    {
        plan(player);
    }
}

NAVIGATOR_REGISTER(RRTNavigator)
