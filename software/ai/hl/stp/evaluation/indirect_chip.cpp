#include "indirect_chip.h"
#include <cmath>
#include <cstddef>
#include <vector>
#include "ai/common/field.h"
#include "ai/hl/stp/evaluation/move.h"
#include "ai/hl/util.h"
#include "ai/hl/world.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "geom/shapes.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace std;
using namespace AI::HL::W;
using namespace AI::HL::STP;
using namespace AI::HL::Util;
using namespace AI::HL::STP::Evaluation;
using namespace Geom;

namespace
{
typedef Point Vector2;
template <size_t N>
using Poly     = std::array<Vector2, N>;
using Triangle = Poly<3>;

DoubleParam CHIP_TARGET_FRACTION(
    u8"chip_dist_fraction.  adjusts how far between ball and target the player "
    u8"will try chip",
    u8"AI/HL/STP/Tactic/indirect_chip", 5.0 / 10.0, 0.0, 100.0);
// DoubleParam CHIP_POWER_BOUNCE_THRESHOLD(u8"chip dist bounce threshold. the
// max fraction of dist between chipper and target the first bounce should be so
// ball is rolling when it reaches the target",
// u8"AI/HL/STP/Tactic/indirect_chip", 7.5/10.0, 0.0, 5.0);
DoubleParam CHIP_POWER_BOUNCE_THRESHOLD(
    u8"chip dist bounce threshold", u8"AI/HL/STP/Tactic/indirect_chip",
    7.5 / 10.0, 0.0, 5.0);
DoubleParam MAX_CHIP_POWER(
    u8"max power the robot can chip the ball at without problems",
    u8"AI/HL/STP/Tactic/indirect_chip", 8.0, 0.0, 100.0);
DoubleParam CHIP_TARGET_AREA_INSET(
    u8"the closest distance to the edge of the field the robot could "
    u8"chip-and-chase to",
    u8"AI/HL/STP/Tactic/indirect_chip", 0.3, 0.0, 100.0);
DoubleParam MIN_CHIP_TRI_AREA(
    u8"the min area of the chip target triangle required to be valid",
    u8"AI/HL/STP/Tactic/indirect_chip", 0.5, 0.0, 100.0);
DoubleParam MIN_CHIP_TRI_EDGE_LEN(
    u8"the min edge len of the chip target tri required to be valid",
    u8"AI/HL/STP/Tactic/indirect_chip", 0.8, 0.0, 100.0);
DoubleParam MIN_CHIP_TRI_EDGE_ANGLE(
    u8"the min angle (deg) between chip triangles edges required to be valid",
    u8"AI/HL/STP/Tactic/indirect_chip", 20, 0.0, 180);
DoubleParam CHIP_CHERRY_POWER_DOWNSCALE(
    u8"percentage of distance to center of triangle to return as target",
    u8"AI/HL/STP/Tactic/indirect_chip", 0.85, 0.0, 100.0);
}

std::pair<Point, Angle> AI::HL::STP::Evaluation::indirect_chip_target(
    World world, Player player)
{
    Point enemy_goal_positive =
        world.field().enemy_goal_boundary().first.x > 0.0
            ? world.field().enemy_goal_boundary().first
            : world.field().enemy_goal_boundary().second;
    Point enemy_goal_negative =
        world.field().enemy_goal_boundary().first.x < 0.0
            ? world.field().enemy_goal_boundary().first
            : world.field().enemy_goal_boundary().second;

    // The triangle formed by the enemy goalposts and the ball. Any robots in
    // this triangle could block a chip/shot
    Triangle chip_target_area = triangle(
        world.ball().position(), enemy_goal_positive, enemy_goal_negative);
    double rough_chip_dist =
        (world.field().enemy_goal() - world.ball().position()).len() *
        (CHIP_TARGET_FRACTION * 0.9);

    std::vector<Vector2> blocking_robots;

    // Adds any enemies that are in the block triangle and further than the chip
    // dist to the blocking_robots vector
    for (auto i : world.enemy_team())
    {
        if ((contains(chip_target_area, i.position()) ||
             offset_to_line(
                 world.ball().position(), enemy_goal_negative, i.position()) <=
                 Robot::MAX_RADIUS ||
             offset_to_line(
                 world.ball().position(), enemy_goal_positive, i.position()) <=
                 Robot::MAX_RADIUS) &&
            ((i.position() - world.ball().position()).len() >
             rough_chip_dist + Robot::MAX_RADIUS))
        {
            blocking_robots.push_back(i.position());
        }
    }

    // Adds any friendly robots that are in the block triangle and further than
    // the chip dist to the blocking_robots vector
    for (auto i : world.friendly_team())
    {
        if ((contains(chip_target_area, i.position()) ||
             offset_to_line(
                 world.ball().position(), enemy_goal_negative, i.position()) <=
                 Robot::MAX_RADIUS ||
             offset_to_line(
                 world.ball().position(), enemy_goal_positive, i.position()) <=
                 Robot::MAX_RADIUS) &&
            ((i.position() - world.ball().position()).len() >
             rough_chip_dist + Robot::MAX_RADIUS))
        {
            blocking_robots.push_back(i.position());
        }
    }

    // should not consider first blocker
    Point chip_net_target =
        angle_sweep_circles(
            world.ball().position(), enemy_goal_positive, enemy_goal_negative,
            blocking_robots, Robot::MAX_RADIUS)
            .first;
    Angle chip_angle =
        angle_sweep_circles(
            world.ball().position(), enemy_goal_positive, enemy_goal_negative,
            blocking_robots, Robot::MAX_RADIUS)
            .second;
    Point chip_dir =
        chip_net_target - world.ball().position();  // from ball to point in net
    double total_chip_dist =
        chip_dir.len();  // dist between ball an target in net
    Point max_target =
        world.ball().position() +
        chip_dir.norm(total_chip_dist * CHIP_POWER_BOUNCE_THRESHOLD);
    Point target;

    // Chipper is in our half of the field. Set target to center of net so no
    // carpeting
    if (player.position().x < 0.0)
    {
        target = world.ball().position() +
                 (world.field().enemy_goal() - world.ball().position())
                     .norm(total_chip_dist * CHIP_TARGET_FRACTION);

        // target should never be further than max_target
        if ((target - world.ball().position()).len() -
                (max_target - world.ball().position()).len() >
            0.0)
            target = world.ball().position() +
                     (world.field().enemy_goal() - world.ball().position())
                         .norm(total_chip_dist * CHIP_POWER_BOUNCE_THRESHOLD);

        // target should never be furhter than MAX_CHIP_POWER
        if ((target - world.ball().position()).len() > MAX_CHIP_POWER)
            target = world.ball().position() +
                     (world.field().enemy_goal() - world.ball().position())
                         .norm(MAX_CHIP_POWER);
    }
    else
    {  // Try to chip for a (indirect) goal
        target = world.ball().position() +
                 (chip_dir.norm(total_chip_dist * CHIP_TARGET_FRACTION));

        // target should never be further than max_target
        if ((target - world.ball().position()).len() -
                (max_target - world.ball().position()).len() >
            0.0)
            target =
                world.ball().position() +
                chip_dir.norm(total_chip_dist * CHIP_POWER_BOUNCE_THRESHOLD);

        // target should never be furhter than MAX_CHIP_POWER
        if ((target - world.ball().position()).len() > MAX_CHIP_POWER)
            target = world.ball().position() + chip_dir.norm(MAX_CHIP_POWER);
    }

    // LOGF_INFO(u8"Chipper can see: %1/1 of net, CHIP_DIST: %2", chip_angle,
    // (target - world.ball().position()).len());
    return std::make_pair(target, chip_angle);
}

Point AI::HL::STP::Evaluation::deflect_off_enemy_target(World world)
{
    Point enemy_goal_positive =
        world.field().enemy_goal_boundary().first.x > 0.0
            ? world.field().enemy_goal_boundary().first
            : world.field().enemy_goal_boundary().second;
    Point enemy_goal_negative =
        world.field().enemy_goal_boundary().first.x < 0.0
            ? world.field().enemy_goal_boundary().first
            : world.field().enemy_goal_boundary().second;

    // The triangle formed by the enemy goalposts and the ball. Any robots in
    // this triangle could block a chip/shot
    Triangle chip_target_area = triangle(
        world.ball().position(), enemy_goal_positive, enemy_goal_negative);

    Robot enemyClosestToEdge = world.enemy_team()[0];
    double shortestLenToEdge = 100.0;
    double closestEdgeY;
    // Finds the y value of the closest edge of the field (likely where the ball
    // is)
    if (world.ball().position().y <= 0.0)
        closestEdgeY = world.field().friendly_corner_neg().y;
    else
        closestEdgeY = world.field().friendly_corner_pos().y;

    // Find the enemy that's blocking a shot that's closest to the edge of the
    // field
    for (auto i : world.enemy_team())
    {
        if ((contains(chip_target_area, i.position()) ||
             offset_to_line(
                 world.ball().position(), enemy_goal_negative, i.position()) <=
                 Robot::MAX_RADIUS ||
             offset_to_line(
                 world.ball().position(), enemy_goal_positive, i.position()) <=
                 Robot::MAX_RADIUS) &&
            (i.position().x > world.ball().position().x))
        {
            if (abs(i.position().y - closestEdgeY) < shortestLenToEdge)
            {
                enemyClosestToEdge = i;
                shortestLenToEdge  = abs(i.position().y - closestEdgeY);
            }
        }
    }

    // want to shoot at the edge of a robot so the ball deflects towards the
    // edge of the field
    Point dir     = enemyClosestToEdge.position() - world.ball().position();
    Point dirPerp = dir.perp().norm(Robot::MAX_RADIUS * 0.75);
    Point target  = Point(0, 0);

    // choose point closest to edge of field
    if (abs((world.ball().position() + dir + dirPerp).y - closestEdgeY) >
        abs((world.ball().position() + dir - dirPerp).y - closestEdgeY))
        target = world.ball().position() + dir - dirPerp;
    else
        target = world.ball().position() + dir + dirPerp;

    return target;
}

std::pair<Point, bool> AI::HL::STP::Evaluation::indirect_chipandchase_target(
    World world)
{
    // creates a vector of all non-goalie enemy robots
    Robot enemy_goalie = world.enemy_team()[world.enemy_team().goalie()];
    std::vector<Point> non_goalie_enemy_positions;
    std::vector<Point> all_enemy_positions;
    for (auto i : world.enemy_team())
    {
        all_enemy_positions.push_back(i.position());
        if (i != enemy_goalie)
        {
            non_goalie_enemy_positions.push_back(i.position());
        }
    }

    // LOGF_INFO(u8"NUMBER OF NON_GOALIE_ENEMIES: %1",
    // non_goalie_enemy_positions.size());
    std::vector<Triangle> allTriangles =
        AI::HL::STP::Evaluation::get_all_triangles(
            world, non_goalie_enemy_positions);
    // LOGF_INFO(u8"NUMBER OF ALLTRIANGLES: %1", allTriangles.size());
    std::vector<Triangle> target_triangles =
        AI::HL::STP::Evaluation::filter_open_triangles(
            allTriangles, all_enemy_positions);
    // LOGF_INFO(u8"NUMBER OF TARGETTRIANGLES AFTER FILTEROPEN: %1",
    // target_triangles.size());
    target_triangles = AI::HL::STP::Evaluation::remove_outofbounds_triangles(
        world, target_triangles);
    // LOGF_INFO(u8"NUMBER OF TRIANGLES AFTER REMOVING OUTOFBOUNDS: %1",
    // target_triangles.size());

    if (!target_triangles.empty())
    {
        std::pair<Triangle, bool> largest_triangle =
            AI::HL::STP::Evaluation::get_largest_triangle(
                target_triangles, MIN_CHIP_TRI_AREA, MIN_CHIP_TRI_EDGE_LEN);
        Triangle t = largest_triangle.first;
        bool valid = largest_triangle.second;
        Point target =
            AI::HL::STP::Evaluation::get_triangle_center_and_area(t).first;
        target = target.norm(
            (target - world.ball().position()).len() *
            CHIP_CHERRY_POWER_DOWNSCALE);

        // target should never be furhter than MAX_CHIP_POWER
        if ((target - world.ball().position()).len() > MAX_CHIP_POWER)
            target = world.ball().position() +
                     (target - world.ball().position()).norm(MAX_CHIP_POWER);

        // LOGF_INFO(u8"EVALUATING CHIP AND CHASE, VALID: %1", valid);
        // LOGF_INFO(u8"TRIANGLE PTS: %1, %2, %3", t[0], t[1], t[2]);

        return std::make_pair(target, valid);
    }
    else
    {
        // LOG_INFO(u8"NO VALID TRIANGLES FOR CHIP AND CHASE");
        return std::make_pair(world.field().enemy_goal(), false);
    }
}

std::vector<Triangle> AI::HL::STP::Evaluation::get_all_triangles(
    World world, std::vector<Point> enemy_players)
{
    std::vector<Triangle> triangles;
    // std::vector<Point> corners =
    // AI::HL::STP::Evaluation::get_chip_target_area_corners(world,
    // CHIP_TARGET_AREA_INSET);
    std::vector<Point> allPts = enemy_players;
    allPts.push_back(world.field().enemy_corner_neg());
    allPts.push_back(world.field().enemy_corner_pos());
    allPts.push_back(Point(0, world.field().enemy_corner_pos().y));
    allPts.push_back(Point(0, world.field().enemy_corner_neg().y));
    // allPts.insert(std::end(allPts), std::begin(corners), std::end(corners));

    for (unsigned int i = 0; i < allPts.size() - 2; i++)
    {
        for (unsigned int j = i + 1; j < allPts.size() - 1; j++)
        {
            for (unsigned int k = j + 1; k < allPts.size(); k++)
            {
                Point p1   = allPts[i];
                Point p2   = allPts[j];
                Point p3   = allPts[k];
                Triangle t = triangle(p1, p2, p3);
                triangles.push_back(t);
            }
        }
    }

    return triangles;
}

std::pair<Triangle, bool> AI::HL::STP::Evaluation::get_largest_triangle(
    std::vector<Triangle> allTriangles, double min_area, double min_edge_len,
    double min_edge_angle)
{
    Triangle largest = allTriangles[0];
    double largest_area =
        AI::HL::STP::Evaluation::get_triangle_center_and_area(largest).second;

    bool valid = false;

    for (unsigned int i = 0; i < allTriangles.size(); i++)
    {
        Triangle t = allTriangles[i];
        double area =
            AI::HL::STP::Evaluation::get_triangle_center_and_area(t).second;
        double l1 = (t[1] - t[0]).len();
        double l2 = (t[2] - t[0]).len();
        double l3 = (t[2] - t[1]).len();

        Angle a1 = vertex_angle(t[1], t[0], t[2]).angle_mod().abs();
        Angle a2 = vertex_angle(t[0], t[1], t[2]).angle_mod().abs();
        Angle a3 = vertex_angle(t[0], t[2], t[1]).angle_mod().abs();

        if (area > largest_area && area >= min_area && l1 >= min_edge_len &&
            l2 >= min_edge_len && l3 >= min_edge_len &&
            a1.to_degrees() >= min_edge_angle &&
            a2.to_degrees() >= min_edge_angle &&
            a3.to_degrees() >= min_edge_angle)
        {
            largest      = t;
            largest_area = area;
            valid        = true;
        }
    }

    return std::make_pair(largest, valid);
}

std::vector<Triangle> AI::HL::STP::Evaluation::filter_open_triangles(
    std::vector<Triangle> triangles, std::vector<Point> enemy_players)
{
    std::vector<Triangle> filtered_triangles;
    bool containsEnemy = false;
    for (unsigned int i = 0; i < triangles.size(); i++)
    {
        // create a triangle that is slightly smaller so the robots making up
        // the vertices
        // are not counted as in the triangle
        Point p1 = triangles[i][0] +
                   ((AI::HL::STP::Evaluation::get_triangle_center_and_area(
                         triangles[i])
                         .first) -
                    triangles[i][0])
                       .norm(2.5 * Robot::MAX_RADIUS);
        Point p2 = triangles[i][1] +
                   ((AI::HL::STP::Evaluation::get_triangle_center_and_area(
                         triangles[i])
                         .first) -
                    triangles[i][1])
                       .norm(2.5 * Robot::MAX_RADIUS);
        Point p3 = triangles[i][2] +
                   ((AI::HL::STP::Evaluation::get_triangle_center_and_area(
                         triangles[i])
                         .first) -
                    triangles[i][2])
                       .norm(2.5 * Robot::MAX_RADIUS);
        Triangle t    = triangle(p1, p2, p3);
        containsEnemy = false;
        for (unsigned int k = 0; k < enemy_players.size(); k++)
        {
            if (contains(t, enemy_players[k]) == true)
            {
                containsEnemy = true;
                break;
            }
        }

        if (containsEnemy == false)
        {
            filtered_triangles.push_back(triangles[i]);
        }
    }

    return filtered_triangles;
}

std::vector<Triangle> AI::HL::STP::Evaluation::remove_outofbounds_triangles(
    World world, std::vector<Triangle> triangles)
{
    std::vector<Triangle> valid_triangles;
    std::vector<Point> chip_area_corners =
        AI::HL::STP::Evaluation::get_chip_target_area_corners(
            world, CHIP_TARGET_AREA_INSET);
    Point center;

    double smallest_x = chip_area_corners[0].x;
    double smallest_y = chip_area_corners[0].y;
    double largest_x  = chip_area_corners[0].x;
    double largest_y  = chip_area_corners[0].y;

    for (unsigned int i = 0; i < chip_area_corners.size(); i++)
    {
        Point p = chip_area_corners[i];
        if (p.x < smallest_x)
            smallest_x = p.x;
        if (p.x > largest_x)
            largest_x = p.x;
        if (p.y < smallest_y)
            smallest_y = p.y;
        if (p.y > largest_y)
            largest_y = p.y;
    }

    for (unsigned int i = 0; i < triangles.size(); i++)
    {
        Triangle t = triangles[i];
        center =
            AI::HL::STP::Evaluation::get_triangle_center_and_area(triangles[i])
                .first;
        if (center.x <= largest_x && center.x >= smallest_x &&
            center.y <= largest_y && center.y >= smallest_y)
        {
            valid_triangles.push_back(t);
        }
    }

    return valid_triangles;
}

std::pair<Point, double> AI::HL::STP::Evaluation::get_triangle_center_and_area(
    Triangle triangle)
{
    Point p1 = triangle[0];
    Point p2 = triangle[1];
    Point p3 = triangle[2];

    double center_x = (p1.x + p2.x + p3.x) / 3;
    double center_y = (p1.y + p2.y + p3.y) / 3;

    double area = abs(
        0.5 * ((p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y)));
    Point center = Point(center_x, center_y);

    return std::make_pair(center, area);
}

std::vector<Point> AI::HL::STP::Evaluation::get_chip_target_area_corners(
    World world, double inset)
{
    std::vector<Point> corners;

    double ballX     = world.ball().position().x;
    double fieldX    = world.field().enemy_goal().x - inset;
    double negFieldY = world.field().enemy_corner_neg().y + inset;
    double posFieldY = world.field().enemy_corner_pos().y - inset;

    Point p1 = Point(ballX, negFieldY);
    Point p2 = Point(ballX, posFieldY);
    Point p3 = Point(fieldX, negFieldY);
    Point p4 = Point(fieldX, posFieldY);

    corners.push_back(p1);
    corners.push_back(p2);
    corners.push_back(p3);
    corners.push_back(p4);

    // LOGF_INFO(u8"CHIP_AREA_CORNERS: %1, %2, %3, %4", p1, p2, p3, p4);

    return corners;
}
