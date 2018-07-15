#include "ai/navigator/util.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <util/timestep.h>
#include "ai/flags.h"
#include "ai/navigator/rrt_planner.h"
#include "ai/util.h"
#include "geom/param.h"
#include "geom/rect.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::Flags;
using namespace AI::Nav::W;
using namespace Geom;

using AI::BE::Primitives::PrimitiveDescriptor;

namespace AI
{
namespace Nav
{
DoubleParam PLAYER_AVERAGE_VELOCITY(
    u8"Average Player Velocity", u8"AI/Nav", 1.5, 0.01, 99.0);

namespace RRT
{
extern IntParam jon_hysteris_hack;
}
}
}

namespace
{
DoubleParam POSITION_EPS(
    u8"Position tolerance in destination", u8"AI/Nav", 0.05, 0.01, 0.5);
DoubleParam VELOCITY_EPS(
    u8"Velocity tolerance in destination", u8"AI/Nav", 0.06, 0.001, 0.5);
RadianParam ANGLE_EPS(
    u8"Orientation tolerance in angle", u8"AI/Nav", 5.0, 0.1, 30.0);

DoubleParam INTERCEPT_ANGLE_STEP_SIZE(
    u8"Angle increment in ball approach (deg)", u8"AI/Nav/Util", 10.0, 0.1,
    30.0);

BoolParam OWN_HALF_OVERRIDE(
    u8"Robots must stay in own half", u8"AI/Nav/Util", false);

// small value to ensure non-equivilance with floating point math
// but too small to make a difference in the actual game
constexpr double SMALL_BUFFER = 0.0001;

DoubleParam ENEMY_MOVEMENT_FACTOR(
    u8"Enemy position interp length", u8"AI/Nav/Util", 0.0, 0.0, 2.0);
DoubleParam FRIENDLY_MOVEMENT_FACTOR(
    u8"Friendly movement extrapolation factor", u8"AI/Nav/Util", 1.0, 0.0, 2.0);
DoubleParam GOAL_POST_BUFFER(
    u8"Goal post avoidance dist", u8"AI/Nav/Util", 0.0, -0.2, 0.2);

// zero lets them brush
// positive enforces amount meters away
// negative lets them bump

DoubleParam ENEMY_BUFFER_SHORT(
    u8"Short enemy avoidance dist", u8"AI/Nav/Util", -0.05, -1, 1);
DoubleParam ENEMY_BUFFER(
    u8"Normal enemy avoidance dist", u8"AI/Nav/Util", 0.1, -1, 1);
DoubleParam ENEMY_BUFFER_LONG(
    u8"Long enemy avoidance dist", u8"AI/Nav/Util", 0.2, -1, 1);

DoubleParam FRIENDLY_BUFFER_SHORT(
    u8"Short friendly avoidance dist", u8"AI/Nav/Util", 0.1, -1, 1);
DoubleParam FRIENDLY_BUFFER(
    u8"Normal friendly avoidance dist", u8"AI/Nav/Util", 0.2, -1, 1);
DoubleParam FRIENDLY_BUFFER_LONG(
    u8"Long friendly avoidance dist", u8"AI/Nav/Util", 0.3, -1, 1);
DoubleParam FRIENDLY_ROBOT_DECEL(
    u8"Friendly robot braking decel", u8"AI/Nav/Util", 3.0, 0.0, 10.0);

DoubleParam PASS_CHALLENGE_BUFFER(
    u8"Intercept challenge friendly avoidance", u8"AI/Nav/Util", 1.0, 0.1, 2.0);

// This buffer is in addition to the robot radius
DoubleParam BALL_TINY_BUFFER(
    u8"Small ball avoidance dist", u8"AI/Nav/Util", 0.05, -1, 1);
DoubleParam BALL_REGULAR_BUFFER(
    u8"Regular ball avoidance dist", u8"AI/Nav/Util", 0.16, 0, 1);

// This buffer is in addition to the robot radius
DoubleParam DEFENSE_AREA_BUFFER(
    u8"Defense avoidance dist", u8"AI/Nav/Util", 0, -1, 1);

// this is by how much we should stay away from the playing boundry
DoubleParam PLAY_AREA_BUFFER(
    u8"Field boundary avoidance dist", u8"AI/Nav/Util", 0, 0, 1);
DoubleParam OWN_HALF_BUFFER(
    u8"Enemy half avoidance dist if enabled", u8"AI/Nav/Util", 0, 0, 1.0);
DoubleParam TOTAL_BOUNDS_BUFFER(
    u8"Ref area avoidance dist", u8"AI/Nav/Util", -0.18, -1, 1);

DoubleParam PENALTY_KICK_BUFFER(
    u8"Ball avoidance dist during penalty kick (rule=0.4) ", u8"AI/Nav/Util",
    0.4, 0, 1.0);

DoubleParam FRIENDLY_KICK_BUFFER(
    u8"Friendly kick avoidance dist (rule=0.2)", u8"AI/Nav/Util", 0.2, 0, 1.0);

BoolParam CUSTOM_X_BOUNDS_ENABLE(
        u8"Whether or not to enable custom bounds for testing", u8"AI/Nav/CustomStuff", false);
DoubleParam MIN_X_BOUND(
        u8"The minimum x coordinate of the area our robots may occupy", u8"AI/Nav/CustomStuff", -5.0, -10.0, 10.0);
DoubleParam MAX_X_BOUND(
        u8"The maximum x coordinate of the area our robots may occupy", u8"AI/Nav/CustomStuff", 5.0, -10.0, 10.0);

constexpr double RAM_BALL_ALLOWANCE = 0.05;

constexpr double BALL_STOP = 0.05;
// distance from the ball's future position before we start heading towards the
// ball
constexpr double CATCH_BALL_THRESHOLD = 0.1;
// distance behind the ball's future position that we should aim for when
// catching the ball
constexpr double CATCH_BALL_DISTANCE_AWAY = 0.1;
// if the ball velocity is below this value then act as if it isn't moving
constexpr double CATCH_BALL_VELOCITY_THRESH = 0.05;

// this structure determines how far away to stay from a prohibited point or
// line-segment
double play_area()
{
    return /*(2 * AI::Nav::W::Player::MAX_RADIUS)*/ +PLAY_AREA_BUFFER;
}
double total_bounds_area()
{
    return TOTAL_BOUNDS_BUFFER;
}
double enemy(AI::Nav::W::World world, AI::Nav::W::Robot player)
{
    if (world.enemy_team().size() <= 0)
    {
        return 0.0;
    }
    double buffer = 0.0;

    switch (player.avoid_distance())
    {
        case AvoidDistance::SHORT:
            buffer = ENEMY_BUFFER_SHORT;
            break;

        case AvoidDistance::MEDIUM:
            buffer = ENEMY_BUFFER;
            break;

        case AvoidDistance::LONG:
            buffer = ENEMY_BUFFER_LONG;
            break;
    }

    return player.MAX_RADIUS + buffer;
}

double goal_post(AI::Nav::W::Player player)
{
    return Ball::RADIUS + player.MAX_RADIUS + GOAL_POST_BUFFER;
}

double friendly(AI::Nav::W::Player player, MovePrio obs_prio = MovePrio::MEDIUM)
{
    MovePrio player_prio = player.prio();
    double buffer        = FRIENDLY_BUFFER;
    if (obs_prio < player_prio)
    {
        buffer = FRIENDLY_BUFFER_SHORT;
    }
    else if (player_prio < obs_prio)
    {
        buffer = FRIENDLY_BUFFER_LONG;
    }

    if (player.avoid_distance() == AvoidDistance::LONG)
    {
        buffer = PASS_CHALLENGE_BUFFER;
    }

    return player.MAX_RADIUS + buffer;
}
double ball_stop(AI::Nav::W::Player player)
{
    return Ball::RADIUS + player.MAX_RADIUS + AI::Util::BALL_STOP_DIST;
}
double ball_tiny(AI::Nav::W::Player player)
{
    return Ball::RADIUS + player.MAX_RADIUS + BALL_TINY_BUFFER;
}
double ball_regular(AI::Nav::W::Player player)
{
    return Ball::RADIUS + player.MAX_RADIUS + BALL_REGULAR_BUFFER;
}
double friendly_defense(AI::Nav::W::World world, AI::Nav::W::Player player)
{
    return world.field().defense_area_width() + player.MAX_RADIUS +
           DEFENSE_AREA_BUFFER;
}
double friendly_kick(AI::Nav::W::World world, AI::Nav::W::Player player)
{
    return world.field().defense_area_width() + player.MAX_RADIUS +
           FRIENDLY_KICK_BUFFER;
}
double own_half(AI::Nav::W::Player player)
{
    return player.MAX_RADIUS + OWN_HALF_BUFFER;
}
double penalty_kick_friendly(AI::Nav::W::Player player)
{
    return player.MAX_RADIUS + PENALTY_KICK_BUFFER + Ball::RADIUS;
}
double penalty_kick_enemy(AI::Nav::W::Player player)
{
    return player.MAX_RADIUS + PENALTY_KICK_BUFFER + Ball::RADIUS;
}

double get_net_trespass(Point cur, Point dst, AI::Nav::W::World world)
{
    double violate = 0.0;
    double circle_radius =
        world.field().total_length() / 2.0 - world.field().length() / 2.0;
    Point A(
        -world.field().total_length() / 2.0, world.field().goal_width() / 2.0);
    Point B(
        -world.field().total_length() / 2.0, -world.field().goal_width() / 2.0);
    double sdist = dist(Seg(A, B), Seg(cur, dst));
    violate      = std::max(violate, circle_radius - sdist);
    return violate;
}

double get_goal_post_trespass(
    Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    double violate       = 0.0;
    double circle_radius = goal_post(player);
    Point A(world.field().length() / 2.0, world.field().goal_width() / 2.0);
    Point B(world.field().length() / 2.0, -world.field().goal_width() / 2.0);
    Point C(-world.field().length() / 2.0, world.field().goal_width() / 2.0);
    Point D(-world.field().length() / 2.0, -world.field().goal_width() / 2.0);
    Point pts[4] = {A, B, C, D};
    for (Point i : pts)
    {
        double sdist = dist(i, Seg(cur, dst));
        violate      = std::max(violate, circle_radius - sdist);
    }
    return violate;
}

double get_enemy_trespass(Point cur, Point dst, AI::Nav::W::World world)
{
    double violate = 0.0;
    // avoid enemy robots
    for (AI::Nav::W::Robot rob : world.enemy_team())
    {
        double circle_radius = enemy(world, rob);
        double sdist         = dist(
            Seg(rob.position(),
                rob.position() + ENEMY_MOVEMENT_FACTOR * rob.velocity()),
            Seg(cur, dst));
        violate = std::max(violate, circle_radius - sdist);
    }

    return violate;
}

double get_play_area_boundary_trespass(
    Point cur, Point dst, AI::Nav::W::World world)
{
    return 0.0;
    const Field &f = world.field();
    Point sw_corner(-f.length() / 2, -f.width() / 2);
    Rect bounds(sw_corner, f.length(), f.width());
    bounds.expand(-play_area());
    double violation = 0.0;
    if (!bounds.point_inside(cur))
    {
        violation = std::max(violation, bounds.dist_to_boundary(cur));
    }
    if (!bounds.point_inside(dst))
    {
        violation = std::max(violation, bounds.dist_to_boundary(dst));
    }

    return violation;
}

double get_total_bounds_trespass(Point cur, Point dst, AI::Nav::W::World world)
{
    return 0.0;
    const Field &f = world.field();
    Point sw_corner(-f.total_length() / 2, -f.total_width() / 2);
    Rect bounds(sw_corner, f.total_length(), f.total_width());
    bounds.expand(-total_bounds_area());
    double violation = 0.0;
    if (!bounds.point_inside(cur))
    {
        violation = std::max(violation, bounds.dist_to_boundary(cur));
    }
    if (!bounds.point_inside(dst))
    {
        violation = std::max(violation, bounds.dist_to_boundary(dst));
    }
    if(CUSTOM_X_BOUNDS_ENABLE) {
//        std::cout << "\n\n IN ENABLE\n\n" << std::endl;
        violation = std::max((MIN_X_BOUND - dst.x), violation);
        violation = std::max((dst.x - MAX_X_BOUND), violation);
    }
    return violation;
}

double get_friendly_trespass(
    Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    double violate = 0.0;
    // avoid enemy robots
    for (AI::Nav::W::Player rob : world.friendly_team())
    {
        if (rob == player)
        {
            continue;
        }

        double braking_dist = (rob.velocity().lensq() / 2 * FRIENDLY_ROBOT_DECEL)
                              * (1 / TIMESTEPS_PER_SECOND) * FRIENDLY_MOVEMENT_FACTOR;
        Seg braking_line = Seg(rob.position(), rob.position() + rob.velocity().norm() * braking_dist);

        Seg path_seg = Seg(cur, dst);

        double exclusion_radius_violation =
                (dist(path_seg, rob.position()) < FRIENDLY_BUFFER_LONG) ?
                FRIENDLY_BUFFER_LONG - dist(path_seg, rob.position()) :
                0;

        double braking_line_violation =
                (dist(path_seg, braking_line) < FRIENDLY_BUFFER) ?
                FRIENDLY_BUFFER - dist(path_seg, braking_line) :
                0;

        violate = exclusion_radius_violation + braking_line_violation; 
    }
    return violate;
}

double get_ball_stop_trespass(
    Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    double violate       = 0.0;
    const Ball &ball     = world.ball();
    double circle_radius = ball_stop(player);
    double sdist         = dist(ball.position(), Seg(cur, dst));
    violate              = std::max(violate, circle_radius - sdist);
    return violate;
}

double get_area_trespass(Point cur, Point dst, Point goal, Rect bounds, Player player)
{
    bounds.expand(player.MAX_RADIUS);
    Point proj = (goal - cur).project((dst - cur)) + cur;
    double violation = 0.0;
    if (bounds.point_inside(cur))
    {
    	violation = std::max(violation, bounds.dist_to_boundary(cur));
    }
    if (bounds.point_inside(dst))
    {
    	violation = std::max(violation, bounds.dist_to_boundary(dst));
    }
    if (bounds.point_inside(proj)) {
        violation = std::max(violation, bounds.dist_to_boundary(proj));
    }
    return violation;
}

double get_offense_area_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    return get_area_trespass(cur, dst, world.field().enemy_goal(), world.field().enemy_crease(), player);
}

double get_defense_area_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    return get_area_trespass(cur, dst, world.field().friendly_goal(), world.field().friendly_crease(), player);
}

double get_own_half_trespass(
    Point, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    const Field &f = world.field();
    Point p(-f.total_length() / 2, -f.total_width() / 2);
    Rect bounds(p, f.total_length() / 2, f.total_width());
    bounds.expand(-own_half(player));
    double violation = 0.0;
    if (!bounds.point_inside(dst))
    {
        violation = std::max(violation, bounds.dist_to_boundary(dst));
    }
    return violation;
}

double get_ball_tiny_trespass(
    Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    const Ball &ball     = world.ball();
    double circle_radius = ball_tiny(player);
    double sdist         = dist(Seg(cur, dst), ball.position());
    return std::max(0.0, circle_radius - sdist);
}

double get_ball_regular_trespass(
    Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    const Ball &ball     = world.ball();
    double circle_radius = ball_regular(player);
    double sdist         = dist(Seg(cur, dst), ball.position());
    return std::max(0.0, circle_radius - sdist);
}

double get_penalty_friendly_trespass(
    Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    const Ball &ball = world.ball();
    const Field &f   = world.field();
    Point a(
        ball.position().x - penalty_kick_friendly(player),
        -f.total_width() / 2);
    Point b(f.total_length() / 2, f.total_width() / 2);
    Rect bounds(a, b);
    double violation = 0.0;
    if (!bounds.point_inside(cur))
    {
        violation = std::max(violation, bounds.dist_to_boundary(cur));
    }
    if (!bounds.point_inside(dst))
    {
        violation = std::max(violation, bounds.dist_to_boundary(dst));
    }
    return violation;
}

double get_penalty_enemy_trespass(
    Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    const Ball &ball = world.ball();
    const Field &f   = world.field();
    Point a(
        ball.position().x + penalty_kick_enemy(player), -f.total_width() / 2);
    Point b(f.total_length() / 2, f.total_width() / 2);
    Rect bounds(a, b);
    double violation = 0.0;
    if (!bounds.point_inside(cur))
    {
        violation = std::max(violation, bounds.dist_to_boundary(cur));
    }
    if (!bounds.point_inside(dst))
    {
        violation = std::max(violation, bounds.dist_to_boundary(dst));
    }
    return violation;
}

struct Violation final
{
    double enemy, friendly, play_area, ball_stop, ball_tiny, ball_regular,
        friendly_defense, enemy_defense, own_half, penalty_kick_friendly,
        penalty_kick_enemy, goal_post, total_bounds, net_allowance;

    MoveFlags extra_flags;

    // set the amount of violation that the player currently has
    void set_violation_amount(
        Point cur, Point dst, AI::Nav::W::World world,
        AI::Nav::W::Player player)
    {
        friendly      = get_friendly_trespass(cur, dst, world, player);
        enemy         = get_enemy_trespass(cur, dst, world);
        goal_post     = get_goal_post_trespass(cur, dst, world, player);
        total_bounds  = get_total_bounds_trespass(cur, dst, world);
        net_allowance = get_net_trespass(cur, dst, world);
        AI::Flags::MoveFlags flags = player.flags() | extra_flags;

        if ((flags & MoveFlags::CLIP_PLAY_AREA) != MoveFlags::NONE)
        {
            play_area = get_play_area_boundary_trespass(cur, dst, world);
        }
        if ((flags & MoveFlags::AVOID_BALL_STOP) != MoveFlags::NONE)
        {
            ball_stop = get_ball_stop_trespass(cur, dst, world, player);
        }
        if ((flags & MoveFlags::AVOID_BALL_TINY) != MoveFlags::NONE)
        {
            ball_tiny = get_ball_tiny_trespass(cur, dst, world, player);
        }
        if ((flags & MoveFlags::AVOID_BALL_MEDIUM) != MoveFlags::NONE)
        {
            ball_regular = get_ball_regular_trespass(cur, dst, world, player);
        }
        if ((flags & MoveFlags::AVOID_FRIENDLY_DEFENSE) != MoveFlags::NONE)
        {
            friendly_defense = get_defense_area_trespass(cur, dst, world, player);
        }
        if ((flags & MoveFlags::AVOID_ENEMY_DEFENSE) != MoveFlags::NONE)
        {
            enemy_defense = get_offense_area_trespass(cur, dst, world, player);
        }
        if ((flags & MoveFlags::STAY_OWN_HALF) != MoveFlags::NONE)
        {
            own_half = get_own_half_trespass(cur, dst, world, player);
        }
        if ((flags & MoveFlags::PENALTY_KICK_FRIENDLY) != MoveFlags::NONE)
        {
            penalty_kick_friendly =
                get_penalty_friendly_trespass(cur, dst, world, player);
        }
        if ((flags & MoveFlags::PENALTY_KICK_ENEMY) != MoveFlags::NONE)
        {
            penalty_kick_enemy =
                get_penalty_enemy_trespass(cur, dst, world, player);
        }
    }

    // default, no violation
    explicit Violation()
        : enemy(0.0),
          friendly(0.0),
          play_area(0.0),
          ball_stop(0.0),
          ball_tiny(0.0),
          ball_regular(0.0),
          friendly_defense(0.0),
          enemy_defense(0.0),
          own_half(0.0),
          penalty_kick_friendly(0.0),
          penalty_kick_enemy(0.0),
          goal_post(0.0),
          total_bounds(0.0),
          net_allowance(0.0),
          extra_flags(MoveFlags::NONE)
    {
    }

    explicit Violation(
        Point cur, Point dst, AI::Nav::W::World world,
        AI::Nav::W::Player player)
        : enemy(0.0),
          friendly(0.0),
          play_area(0.0),
          ball_stop(0.0),
          ball_tiny(0.0),
          ball_regular(0.0),
          friendly_defense(0.0),
          enemy_defense(0.0),
          own_half(0.0),
          penalty_kick_friendly(0.0),
          penalty_kick_enemy(0.0),
          goal_post(0.0),
          total_bounds(0.0),
          net_allowance(0.0),
          extra_flags(MoveFlags::NONE)
    {
        if (OWN_HALF_OVERRIDE)
        {
            extra_flags = extra_flags | MoveFlags::STAY_OWN_HALF;
        }
        set_violation_amount(cur, dst, world, player);
    }

    explicit Violation(
        Point cur, Point dst, AI::Nav::W::World world,
        AI::Nav::W::Player player, MoveFlags added_flags)
        : enemy(0.0),
          friendly(0.0),
          play_area(0.0),
          ball_stop(0.0),
          ball_tiny(0.0),
          ball_regular(0.0),
          friendly_defense(0.0),
          enemy_defense(0.0),
          own_half(0.0),
          penalty_kick_friendly(0.0),
          penalty_kick_enemy(0.0),
          goal_post(0.0),
          total_bounds(0.0),
          net_allowance(0.0),
          extra_flags(added_flags)
    {
        if (OWN_HALF_OVERRIDE)
        {
            extra_flags = extra_flags | MoveFlags::STAY_OWN_HALF;
        }
        set_violation_amount(cur, dst, world, player);
    }

    static Violation get_violation_amount(
        Point cur, Point dst, AI::Nav::W::World world,
        AI::Nav::W::Player player)
    {
        Violation v(cur, dst, world, player);
        return v;
    }

    static Violation get_violation_amount(
        Point cur, Point dst, AI::Nav::W::World world,
        AI::Nav::W::Player player, MoveFlags extra_flags)
    {
        Violation v(cur, dst, world, player, extra_flags);
        return v;
    }

    // whether there is less violation than the violation parameter
    bool no_more_violating_than(Violation b)
    {
        return enemy < b.enemy + Geom::EPS &&
               friendly < b.friendly + Geom::EPS &&
               play_area < b.play_area + Geom::EPS &&
               ball_stop < b.ball_stop + Geom::EPS &&
               ball_tiny < b.ball_tiny + Geom::EPS &&
               friendly_defense < b.friendly_defense + Geom::EPS &&
               enemy_defense < b.enemy_defense + Geom::EPS &&
               own_half < b.own_half + Geom::EPS &&
               penalty_kick_enemy < b.penalty_kick_enemy + Geom::EPS &&
               penalty_kick_friendly < b.penalty_kick_friendly + Geom::EPS &&
               goal_post < b.goal_post + Geom::EPS &&
               total_bounds < b.total_bounds + Geom::EPS &&
               net_allowance < b.net_allowance + Geom::EPS &&
               ball_regular < b.ball_regular + Geom::EPS;
    }

    // whether there are no violations at all
    bool violation_free()
    {
        return enemy < Geom::EPS && friendly < Geom::EPS &&
               play_area < Geom::EPS && ball_stop < Geom::EPS &&
               ball_tiny < Geom::EPS && friendly_defense < Geom::EPS &&
               enemy_defense < Geom::EPS && own_half < Geom::EPS &&
               penalty_kick_enemy < Geom::EPS &&
               penalty_kick_friendly < Geom::EPS && goal_post < Geom::EPS &&
               total_bounds < Geom::EPS && net_allowance < Geom::EPS &&
               ball_regular < Geom::EPS;
    }
};

void process_obstacle(
    std::vector<Point> &ans, AI::Nav::W::World world, AI::Nav::W::Player player,
    Point segA, Point segB, double dist, int num_points)
{
    // we want a regular polygon where the largest inscribed circle
    // has the keepout distance as it's radius
    // circle radius then becomes the radius of the smallest circle that will
    // contain the polygon
    // plus a small buffer
    double radius =
        dist / std::cos(M_PI / static_cast<double>(num_points)) + SMALL_BUFFER;
    double TS  = 2 * num_points * dist * std::tan(M_PI / num_points);
    double TS2 = TS + 2 * (segA - segB).len();
    int n_tot  = num_points * static_cast<int>(std::ceil(TS2 / TS));
    std::vector<Point> temp = seg_buffer_boundaries(segA, segB, radius, n_tot);

    for (Point i : temp)
    {
        if (AI::Nav::Util::valid_dst(i, world, player))
        {
            ans.push_back(i);
        }
    }
}
};

bool AI::Nav::Util::is_done(
    AI::Nav::W::Player player, const PrimitiveDescriptor &desc)
{
    switch (desc.prim)
    {
        case Drive::Primitive::STOP:
            return player.velocity().lensq() < VELOCITY_EPS * VELOCITY_EPS;
        case Drive::Primitive::MOVE:
        case Drive::Primitive::DRIBBLE:
        case Drive::Primitive::SPIN:
        case Drive::Primitive::SHOOT:
            return (player.position() - desc.field_point()).lensq() <
                       POSITION_EPS * POSITION_EPS &&
                   player.velocity().lensq() < VELOCITY_EPS * VELOCITY_EPS;
        default:
            LOG_ERROR(u8"Unhandled primitive");
            return true;
    }
}

bool AI::Nav::Util::has_destination(const PrimitiveDescriptor &desc)
{
    return desc.prim != Drive::Primitive::STOP &&
           desc.prim != Drive::Primitive::CATCH;
}

std::vector<Point> AI::Nav::Util::get_destination_alternatives(
    Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    const int POINTS_PER_OBSTACLE = 6;
    std::vector<Point> ans;
    AI::Flags::MoveFlags flags = player.flags();

    if ((flags & MoveFlags::AVOID_BALL_STOP) != MoveFlags::NONE)
    {
        process_obstacle(
            ans, world, player, dst, dst, friendly(player),
            3 * POINTS_PER_OBSTACLE);
    }

    return ans;
}

bool AI::Nav::Util::valid_dst(
    Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    return Violation::get_violation_amount(dst, dst, world, player)
        .violation_free();
}

bool AI::Nav::Util::valid_path(
    Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    return Violation::get_violation_amount(cur, dst, world, player)
        .no_more_violating_than(
            Violation::get_violation_amount(cur, cur, world, player));
}

bool AI::Nav::Util::valid_path(
    Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player,
    MoveFlags extra_flags)
{
    return Violation::get_violation_amount(cur, dst, world, player, extra_flags)
        .no_more_violating_than(Violation::get_violation_amount(
            cur, cur, world, player, extra_flags));
}

bool AI::Nav::Util::valid_path(
    std::vector<Point> path, AI::Nav::W::World world, AI::Nav::W::Player player, MoveFlags extra_flags)
{
	for(unsigned i=1; i <path.size(); i++){
		if(!valid_path(path[i-1], path[i], world, player, extra_flags)) return false;
	}
	return true;	
}

std::vector<Point> AI::Nav::Util::get_obstacle_boundaries(
    AI::Nav::W::World world, AI::Nav::W::Player player)
{
    return get_obstacle_boundaries(world, player, MoveFlags::NONE);
}

std::vector<Point> AI::Nav::Util::get_obstacle_boundaries(
    AI::Nav::W::World world, AI::Nav::W::Player player, MoveFlags added_flags)
{
    // this number must be >=3
    const int POINTS_PER_OBSTACLE = 6;
    std::vector<Point> ans;
    AI::Flags::MoveFlags flags = player.flags() | added_flags;
    const Field &f             = world.field();

    if ((flags & MoveFlags::AVOID_BALL_STOP) != MoveFlags::NONE)
    {
        process_obstacle(
            ans, world, player, world.ball().position(),
            world.ball().position(), ball_stop(player),
            3 * POINTS_PER_OBSTACLE);
    }

    if ((flags & MoveFlags::STAY_OWN_HALF) != MoveFlags::NONE)
    {
        Point half_point1(0.0, -f.width() / 2);
        Point half_point2(0.0, f.width() / 2);
        process_obstacle(
            ans, world, player, half_point1, half_point2, own_half(player),
            7 * POINTS_PER_OBSTACLE);
    }

    if ((flags & MoveFlags::AVOID_FRIENDLY_DEFENSE) != MoveFlags::NONE)
    {
        Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
        Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
        process_obstacle(
            ans, world, player, defense_point1, defense_point2,
            friendly_defense(world, player), POINTS_PER_OBSTACLE);
    }

    if ((flags & MoveFlags::AVOID_ENEMY_DEFENSE) != MoveFlags::NONE)
    {
        Point defense_point1(f.length() / 2, -f.defense_area_stretch() / 2);
        Point defense_point2(f.length() / 2, f.defense_area_stretch() / 2);
        process_obstacle(
            ans, world, player, defense_point1, defense_point2,
            friendly_kick(world, player), POINTS_PER_OBSTACLE);
    }

    if ((flags & MoveFlags::AVOID_BALL_TINY) != MoveFlags::NONE &&
        (flags & MoveFlags::AVOID_BALL_STOP) == MoveFlags::NONE)
    {
        process_obstacle(
            ans, world, player, world.ball().position(),
            world.ball().position(), ball_tiny(player), POINTS_PER_OBSTACLE);
    }

    if ((flags & MoveFlags::AVOID_BALL_MEDIUM) != MoveFlags::NONE &&
        (flags & MoveFlags::AVOID_BALL_STOP) == MoveFlags::NONE &&
        (flags & MoveFlags::AVOID_BALL_TINY) == MoveFlags::NONE)
    {
        process_obstacle(
            ans, world, player, world.ball().position(),
            world.ball().position(), ball_regular(player), POINTS_PER_OBSTACLE);
    }

    for (AI::Nav::W::Player rob : world.friendly_team())
    {
        if (rob == player)
        {
            // points around self may help with trying to escape when stuck
            // that is why there are double the number of points here
            process_obstacle(
                ans, world, player, rob.position(), rob.position(),
                friendly(player), 2 * POINTS_PER_OBSTACLE);
            continue;
        }
        process_obstacle(
            ans, world, player, rob.position(), rob.position(),
            friendly(player), POINTS_PER_OBSTACLE);
    }

    for (AI::Nav::W::Robot rob : world.enemy_team())
    {
        process_obstacle(
            ans, world, player, rob.position(), rob.position(),
            enemy(world, player), POINTS_PER_OBSTACLE);
    }

    return ans;
}

std::pair<Point, AI::Timestamp> AI::Nav::Util::get_ramball_location(
    Point dst, AI::Nav::W::World world, AI::Nav::W::Player player)
{
    Point ball_dir = world.ball().velocity();

    if (ball_dir.lensq() < Geom::EPS)
    {
        if (dist(world.ball().position(), Seg(player.position(), dst)) <
            RAM_BALL_ALLOWANCE)
        {
            return std::make_pair(
                world.ball().position(), world.monotonic_time());
        }
    }

    if (unique_line_intersect(
            player.position(), dst, world.ball().position(),
            world.ball().position() + ball_dir))
    {
        Point location = line_intersect(
            player.position(), dst, world.ball().position(),
            world.ball().position() + ball_dir);
        AI::Timestamp intersect = world.monotonic_time();
        intersect += std::chrono::duration_cast<AI::Timediff>(
            std::chrono::duration<double>(
                (location - world.ball().position()).len() /
                world.ball().velocity().len()));

        Point vec1 = location - player.position();
        Point vec2 = dst - player.position();

        Point ball_vec = location - world.ball().position();

        if (vec1.dot(vec2) > 0 && ball_dir.dot(ball_vec) != 0.0)
        {
            return std::make_pair(location, intersect);
        }
    }

    // if everything fails then just stay put
    return std::make_pair(player.position(), world.monotonic_time());
}

AI::Timestamp AI::Nav::Util::get_next_ts(
    AI::Timestamp now, Point &p1, Point &p2, Point target_velocity)
{
    double velocity, distance;
    velocity = target_velocity.len();
    distance = (p1 - p2).len();
    return now + std::chrono::duration_cast<AI::Timediff>(
                     std::chrono::duration<double>(distance / velocity));
}

double AI::Nav::Util::estimate_action_duration(
    std::vector<std::pair<Point, Angle>> path_points)
{
    double total_time = 0;
    for (std::size_t i = 0; i < path_points.size() - 1; ++i)
    {
        double dist = (path_points[i].first - path_points[i + 1].first).len();
        total_time += dist / Player::MAX_LINEAR_VELOCITY;
    }
    return total_time;
}

double AI::Nav::Util::calc_mid_vel(Point player_pos, std::vector<Point> plan){
    if(plan.size() < 2) return 0.0;
    
    Point v1 = plan[0] - player_pos;
    Point v2 = plan[1] - plan[0];
    
    double angle = std::acos((v1.norm()).dot(v2.norm())); //should be [0,pi)
    // Slow down if it is a sharp corner
    double angleConstrainedVel =  std::max(0.0, AI::Nav::W::Player::MAX_LINEAR_VELOCITY * std::cos(angle * angle / 1.5));

    // Calculate how long the rest of the path is (starting at the upcoming point) 
    double distRemaining = 0.0;
    for(unsigned i=0;i<plan.size()-1;i++){
        distRemaining += (plan[i+1] - plan[i]).len(); 
    }

    // the maximum speed while still being able to stop before the final point
    double distConstrainedVel = std::sqrt(2*AI::Nav::W::Player::MAX_LINEAR_ACCELERATION*distRemaining);

    return std::min(angleConstrainedVel, distConstrainedVel);
}

#warning needs to be fixed for movement primitives
/*
bool AI::Nav::Util::intercept_flag_handler(AI::Nav::W::World world,
AI::Nav::W::Player player,  AI::Nav::RRT::PlayerData::Ptr player_data) {
        // need to confirm that the player has proper flag

        // need to confirm that the ball is moving at all

        // extract data from the player
        const Field &field = world.field();
        Point target_pos = player.destination().first;
        const Point robot_pos = player.position();

        Angle dest_orientation = player.destination().second;
        // extract information from the ball
        const Ball &ball = world.ball();
        const Rect field_rec({ field.length() / 2, field.width() / 2 }, {
-field.length() / 2, -field.width() / 2 });
        const Point ball_pos = ball.position();
        const Point ball_vel = ball.velocity();
        const Point ball_norm = ball_vel.norm();
        const Point ball_bounded_pos = vector_rect_intersect(field_rec,
ball_pos, ball_pos + ball_norm);
        const Angle target_ball_offset_angle = vertex_angle(ball_pos + ball_vel,
ball_pos, target_pos).angle_mod();

        if (player_data->prev_move_type == player.type() &&
player_data->prev_move_prio == player.prio() && player_data->prev_avoid_distance
== player.avoid_distance()) {
                target_pos = (player_data->previous_dest + target_pos ) /
AI::Nav::RRT::jon_hysteris_hack;
                player_data->previous_dest = target_pos;
                dest_orientation = (player_data->previous_orient +
dest_orientation) / AI::Nav::RRT::jon_hysteris_hack;
                player_data->previous_orient = dest_orientation;
        }


        std::vector<Point> path_points;
        AI::Nav::RRTPlanner planner(world);

//	if (ball_vel.len() < 0.2) {
//		//player.path(get_path_around_ball(world, player, robot_pos,
target_pos, true));
//		intercept_flag_stationary_ball_handler(world, player);
//		return true;
//	}

        // only start rotating around the stationary ball when we're within a
certain distance
        const double dist_to_rotate = 0.25;
        if (ball_vel.len() < 0.4 && (robot_pos - ball_pos).len() <
dist_to_rotate) {
                player.path(get_path_around_ball(world, player, robot_pos,
target_pos, true));
                return true;
        }

        const bool robot_behind_ball = !point_in_front_vector(ball_pos,
ball_vel, robot_pos);
        // find out whether robot is behind the ball or in front of the ball
        if (robot_behind_ball) {
                player.path(get_path_near_ball(world, player,
target_ball_offset_angle));
                return true;
        }

        // if the intersection is off the field or not found for some reason and
the ball is not moving very slow, return failure
        if ((ball.velocity().len() > CATCH_BALL_VELOCITY_THRESH) &&
ball_bounded_pos.x == 0.0 && ball_bounded_pos.y == 0.0) {
                return false;
        }

        // set up the resolution that we should check at
        int points_to_check = 60;
        Point interval = (-ball_pos + ball_bounded_pos) * (1.0 /
points_to_check);
        // set up how much the ball travels in each interval that we check,
assume no decay
        double interval_time = interval.len() / ball.velocity().len();
        MoveFlags flags = AI::Flags::MoveFlags::AVOID_BALL_TINY;

#warning flags and timespec are not accounted for properly
        for (int i = 0; i <= points_to_check; i++) {
                // where the ball would roll to at a later time
                Point ball_future_pos = ball_pos + (interval * i);
                Point dir_from_target = (ball_future_pos -
player.destination().first).norm();

                // get a point that is behind the ball's future position in the
direction of the target, this is where the robot should go to
                Point move_to_point = ball_future_pos + (dir_from_target *
CATCH_BALL_DISTANCE_AWAY);

                // now plan out the path
                path_points = planner.plan(player, move_to_point, flags);
                std::vector<std::pair<Point, Angle>> path_points_with_angle;

                // face the final direction the whole time
                Angle path_orientation = (dir_from_target * -1).orientation();
                path_points_with_angle.push_back(std::make_pair(player.position(),
path_orientation));
                for (Point j : path_points) {
                        path_points_with_angle.push_back(std::make_pair(j,
path_orientation));
                }

                // check if the robot can make it
                if
(AI::Nav::Util::estimate_action_duration(path_points_with_angle) <
(interval_time * i) || (i == points_to_check) || ball.velocity().len() <
CATCH_BALL_VELOCITY_THRESH) {
                        // prepare for assigning the path to the robot
                        AI::Nav::W::Player::Path path;
                        AI::Timestamp working_time = world.monotonic_time();

                        // if we're within a certain threshold then skip this
and just move towards the ball's future position
                        if (Geom::dist(Geom::Seg(move_to_point,
ball_future_pos), player.position()) > CATCH_BALL_THRESHOLD) {
                                // ignore first point since it is bot's position
                                for (unsigned int j = 1; j <
path_points_with_angle.size(); j++) {
                                        // not going for proper timestamp, yet
                                        path.push_back(std::make_pair(path_points_with_angle[j],
working_time));
                                }
                        }

                        // add last point to be the actual point where the ball
should be since we generate a path to a point just behind it
                        path.push_back(std::make_pair(std::make_pair(ball_future_pos,
player.destination().second), working_time));
                        player.path(path);
                        return true;
                }
        }

        // guess we have't found a possible intersecting point
        return false;
}
*/
