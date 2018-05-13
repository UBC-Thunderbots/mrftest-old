#include "ai/hl/stp/evaluation/move.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
using namespace AI::HL::STP;

namespace
{
DoubleParam reduced_radius_small(
    u8"small reduced radius for calculating best shot (robot radius ratio)",
    u8"AI/HL/STP/Shoot", 0.4, 0.0, 1.1);

DoubleParam reduced_radius_big(
    u8"big reduced radius for calculating best shot (robot radius ratio)",
    u8"AI/HL/STP/Shoot", 0.8, 0.0, 1.1);
}

Angle Evaluation::get_shoot_score(
    World world, Player player, bool use_reduced_radius)
{
    double radius;
    if (use_reduced_radius)
    {
        radius = reduced_radius_small;
    }
    else
    {
        radius = reduced_radius_big;
    }

    std::vector<std::pair<Point, Angle>> openings =
        AI::HL::Util::calc_best_shot_all(world, player, radius);

    for (std::vector<std::pair<Point, Angle>>::iterator it = openings.begin();
         it != openings.end(); ++it)
    {
        Angle centre_ang = player.orientation();
        Angle ang_1 =
            (it->first - player.position()).orientation() + it->second / 2.0;
        Angle ang_2 =
            (it->first - player.position()).orientation() - it->second / 2.0;
        if (ang_1.angle_diff(centre_ang) + ang_2.angle_diff(centre_ang) >
            it->second + Angle::of_radians(1e-6))
        {
            continue;
        }
        return std::min(
            ang_1.angle_diff(centre_ang), ang_2.angle_diff(centre_ang));
    }
    return Angle::zero();
}

Point Evaluation::get_best_shot(World world, Robot robot)
{
    return Evaluation::get_best_shot_pair(world, robot).first;
}

std::pair<Point, Angle> Evaluation::get_best_shot_pair(World world, Robot robot)
{
    Point enemy_goal_positive =
        world.field().enemy_goal_boundary().first.x > 0.0
            ? world.field().enemy_goal_boundary().first
            : world.field().enemy_goal_boundary().second;
    Point enemy_goal_negative =
        world.field().enemy_goal_boundary().first.x < 0.0
            ? world.field().enemy_goal_boundary().first
            : world.field().enemy_goal_boundary().second;

    std::vector<Point> obstacles;

    for (auto i : world.enemy_team())
    {
        if ((i.position() - robot.position()).len() > 0.01)
        {
            obstacles.push_back(i.position());
        }
    }
    for (auto i : world.friendly_team())
    {
        if ((i.position() - robot.position()).len() > 0.01)
        {
            obstacles.push_back(i.position());
        }
    }

    return angle_sweep_circles(
        world.ball().position(), enemy_goal_positive, enemy_goal_negative,
        obstacles, Robot::MAX_RADIUS);
}

double Evaluation::get_passee_shoot_score(
    const PassInfo::worldSnapshot& snap, Point position)
{
    std::vector<Point> obstacles;
    Point enemy_goal_positive = snap.enemy_goal_boundary.first.x > 0.0
                                    ? snap.enemy_goal_boundary.first
                                    : snap.enemy_goal_boundary.second;
    Point enemy_goal_negative = snap.enemy_goal_boundary.first.x < 0.0
                                    ? snap.enemy_goal_boundary.first
                                    : snap.enemy_goal_boundary.second;

    for (auto i : snap.enemy_positions)
    {
        obstacles.push_back(i);
    }
    for (auto i : snap.passee_positions)
    {
        if ((i - position).lensq() > 0.005)
        {
            obstacles.push_back(i);
        }
    }

    double angle = angle_sweep_circles(
                       position, enemy_goal_positive, enemy_goal_negative,
                       obstacles, Robot::MAX_RADIUS)
                       .second.to_degrees();

    return 1 / (1 + std::exp(0.2 * (10 - angle)));
}

Evaluation::ShootData Evaluation::evaluate_shoot(
    World world, Player player, bool use_reduced_radius)
{
    ShootData data;

    double radius;
    if (use_reduced_radius)
        radius = reduced_radius_small;
    else
        radius = reduced_radius_big;

    auto shot = AI::HL::Util::calc_best_shot(world, player, radius);

    data.reduced_radius = use_reduced_radius;

    Angle ori          = (shot.first - player.position()).orientation();
    Angle ori_diff     = ori.angle_diff(player.orientation());
    data.accuracy_diff = ori_diff - (shot.second / 2);

    data.target = shot.first;
    data.angle  = shot.second;
#warning where does this variable come from?
    data.can_shoot = data.accuracy_diff < -shoot_accuracy;
    data.blocked   = shot.second == Angle::zero();

#warning a fix to other parts of the code for now
    if (data.blocked)
        data.target = world.field().enemy_goal();

    return data;
}

bool Evaluation::in_shoot_position(World world, Player player, Point target)
{
    Point pos  = player.position();
    Point ball = world.ball().position();

    if ((pos - ball).len() > 1.0)
        return false;
    else if (
        ((ball - pos).orientation() - (target - ball).orientation()).abs() >
        Angle::of_degrees(25.0))
        return false;
    // This line has some sort of state that prevents the robot from ever
    // getting into position.
    // else if (!Plan::valid_path(pos, ball, world, player)) return false;
    return true;
}
