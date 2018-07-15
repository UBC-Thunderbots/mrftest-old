#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/enemy.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/param.h"

#include <vector>

using namespace AI::HL::W;
using namespace AI::HL::STP::Evaluation;
using namespace AI::HL::STP;
using namespace Geom;

namespace
{
BoolParam defense_follow_enemy_baller(
    u8"defense protect against baller", u8"AI/HL/STP/defense", true);

BoolParam goalie_hug_switch(u8"goalie hug switch", u8"AI/HL/STP/defense", true);

DoubleParam max_goalie_dist(
    u8"max goalie dist from goal (robot radius)", u8"AI/HL/STP/defense", 3.0,
    0.0, 10.0);

DoubleParam robot_shrink(
    u8"shrink robot radius", u8"AI/HL/STP/defense", 1.1, 0.1, 2.0);
DoubleParam ball2side_ratio(
    u8"ball2side ratio", u8"AI/HL/STP/defense", 0.7, 0, 10);

BoolParam open_net_dangerous(
    u8"open net enemy is dangerous", u8"AI/HL/STP/defense", true);

DoubleParam tdefend_dist(
    u8"Distance between the tdefenders", u8"AI/HL/STP/tdefend", 2.25, 1.0, 3.0);

// The closest distance players allowed to the ball
// DO NOT make this EXACT, instead, add a little tolerance!
const double AVOIDANCE_DIST =
    AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS + 0.005;

// in ball avoidance, angle between center of 2 robots, as seen from the ball
const Angle AVOIDANCE_ANGLE =
    2.0 * Angle::of_radians(std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST));

/**
 * ssshh... global state
 * DO NOT TOUCH THIS unless you know what you are doing.
 */
bool goalie_top = true;

/*
   template<int N>
   class EvaluateDefense final : public Cacheable<Point,
   CacheableNonKeyArgs<AI::HL::W::World>> {
   protected:
   std::array<Point, N> compute(AI::HL::W::World world) override;
   bool goalie_top;
   };
 */

std::array<Point, MAX_DEFENDERS + 1> waypoints;

std::array<Point, MAX_DEFENDERS + 1> compute(World world)
{
    const Field &field = world.field();

    if (world.ball().position().y > field.goal_width() / 2)
    {
        goalie_top = !goalie_hug_switch;
    }
    else if (world.ball().position().y < -field.goal_width() / 2)
    {
        goalie_top = goalie_hug_switch;
    }

    // list of points to defend, by order of importance
    std::vector<Point> waypoint_defenders;

    // list of points to intercept enemy passing lanes
    std::vector<Point> waypoint_passing_lanes;

    // there is cone ball to goal sides, bounded by 1 rays.
    // the side that goalie is going to guard is goal_side
    // the opposite side is goal_opp
    // a defender will guard goal_opp

    Point ball_pos = world.ball().position();
    if (defense_follow_enemy_baller)
    {
        Robot robot = calc_enemy_baller(world);
        if (robot)
        {
            ball_pos = robot.position();
        }
    }

    const Point goal_side =
        goalie_top ? Point(-field.length() / 2, field.goal_width() / 2)
                   : Point(-field.length() / 2, -field.goal_width() / 2);
    const Point goal_opp =
        goalie_top ? Point(-field.length() / 2, -field.goal_width() / 2)
                   : Point(-field.length() / 2, field.goal_width() / 2);

    // now calculate where you want the goalie to be
    Point waypoint_goalie;

    const double radius = Robot::MAX_RADIUS * robot_shrink;
    bool second_needed  = true;
    {
        // distance on the goalside - ball line that the robot touches
        const Point ball2side   = goal_side - ball_pos;
        const Point touch_vec   = -ball2side.norm();  // side to ball
        const double touch_dist = std::min(
            -ball2side.x * ball2side_ratio,
            max_goalie_dist * Robot::MAX_RADIUS);
        const Point perp = (goalie_top) ? touch_vec.rotate(-Angle::quarter())
                                        : touch_vec.rotate(Angle::quarter());
        waypoint_goalie = goal_side + touch_vec * touch_dist + perp * radius;

        // prevent the goalie from entering the goal area
        waypoint_goalie.x =
            std::max(waypoint_goalie.x, -field.length() / 2 + radius);

        second_needed = dist(waypoint_goalie, Seg(ball_pos, goal_opp)) > radius;
    }

    // first defender will block the remaining cone from the ball
    if (second_needed)
    {
        Point D1 = calc_block_cone_defender(
            goal_side, goal_opp, ball_pos, waypoint_goalie, radius);

        if (ball_on_net(world))
        {
            D1 = calc_block_cone(goal_side, goal_opp, ball_pos, radius);
        }

        bool blowup = false;
        if (D1.x < Robot::MAX_RADIUS - field.length() / 2 +
                       field.defense_area_stretch())
        {
            blowup = true;
        }
        if (std::fabs(D1.y) > field.width() / 4)
        {
            blowup = true;
        }
        if (blowup)
        {
            D1 = (field.friendly_goal() + ball_pos) / 2;
        }
        waypoint_defenders.push_back(D1);
    }

    std::vector<Robot> enemies = enemies_by_grab_ball_dist();

    // sort enemies by distance to own goal
    // std::sort(enemies.begin(), enemies.end(),
    // AI::HL::Util::CmpDist<Robot>(field.friendly_goal()));

    std::vector<Robot> threat;

    if (open_net_dangerous && second_needed)
    {
        std::vector<Point> obstacles;
        obstacles.push_back(waypoint_goalie);
        obstacles.push_back(waypoint_defenders[0]);

        for (size_t i = 0; i < enemies.size(); ++i)
        {
            if (calc_enemy_best_shot_goal(
                    world.field(), obstacles, enemies[i].position())
                    .second > enemy_shoot_accuracy)
            {
                threat.push_back(enemies[i]);
            }
        }
        for (size_t i = 0; i < enemies.size(); ++i)
        {
            if (!(calc_enemy_best_shot_goal(
                      world.field(), obstacles, enemies[i].position())
                      .second > enemy_shoot_accuracy))
            {
                threat.push_back(enemies[i]);
            }
        }
    }
    else
    {
        threat = enemies;
    }

    // figure out positions to stop the passing lanes

    for (size_t i = 0; i < threat.size(); i++)
    {
        waypoint_passing_lanes.push_back(threat[i].position());
    }

    // next two defenders block nearest enemy sights to goal if needed
    // enemies with ball possession are ignored (they should be handled above)
    for (size_t i = 0;
         i < threat.size() && waypoint_defenders.size() < MAX_DEFENDERS; ++i)
    {
        // HACK
        if (defense_follow_enemy_baller)
        {
            Robot robot = calc_enemy_baller(world);
            if (robot && threat[i] == robot)
            {
                continue;
            }
        }

#warning A HACK FOR NOW, may intefere with baller above
        double ball_diff = (ball_pos - threat[i].position()).len();
        if (ball_diff < Robot::MAX_RADIUS + Ball::RADIUS)
        {
            continue;
        }

        // TODO: check if enemy can shoot the ball from here
        // if so, block it

        /**
         * block the passing lanes with the next few robots
         */
        Point D(0, 0);
        if (world.ball().position().x < 0)
        {
            bool blowup = false;
            D           = closest_lineseg_point(
                world.field().friendly_goal(), world.ball().position(),
                threat[i].position());
            if (D.x < Robot::MAX_RADIUS - field.length() / 2 +
                          field.defense_area_stretch())
            {
                blowup = true;
            }
            if (std::fabs(D.y) > field.width() / 4)
            {
                blowup = true;
            }
            if (blowup)
            {
                D = (field.friendly_goal() + threat[i].position()) / 2;
            }
        }
        else
        {
            /*
             * The following block of code calculates the the best place to put
             * extra defenders to block the robots
             * from shooting the ball from one touch passes. instead, we try to
             * block passing lanes
             */
            bool blowup = false;
            D           = calc_block_cone(
                world.ball().position(), world.ball().position(),
                threat[i].position(), radius);
            if (D.x < Robot::MAX_RADIUS - field.length() / 2 +
                          field.defense_area_stretch())
            {
                blowup = true;
            }
            if (std::fabs(D.y) > field.width() / 4)
            {
                blowup = true;
            }
            if (blowup)
            {
                D = (field.friendly_goal() + threat[i].position()) / 2;
            }
        }
        waypoint_defenders.push_back(D);
    }

    // there are too few threat, this is strange
    while (waypoint_defenders.size() < MAX_DEFENDERS)
    {
        waypoint_defenders.push_back((field.friendly_goal() + ball_pos) / 2);
    }

    std::array<Point, MAX_DEFENDERS + 1> waypoints;
    waypoints[0] = waypoint_goalie;
    for (std::size_t i = 0; i < MAX_DEFENDERS; ++i)
    {
        waypoints[i + 1] = waypoint_defenders[i];
    }
    return waypoints;
}

Point tdefender_block_ball(World world, const unsigned index)
{
    Point dirToGoal, target;
    dirToGoal =
        (world.field().friendly_goal() - world.ball().position()).norm();
    target = world.field().friendly_goal() -
             (6 * (index + 1) * Robot::MAX_RADIUS * dirToGoal);
    if (index == 3)
    {
        // same defense layer as tdefender1
        target = world.field().friendly_goal() -
                 (6 * (1.6) * Robot::MAX_RADIUS * dirToGoal);
    }
    Point t = target;
    if (world.ball().position().y < 0.0)
    {
        if (index < 2)
        {
            target = Point(t.x, t.y + tdefend_dist * Robot::MAX_RADIUS);
        }
        else
        {
            target = Point(t.x, t.y - tdefend_dist * Robot::MAX_RADIUS);
        }
    }
    else
    {
        if (index < 2)
        {
            target = Point(t.x, t.y - tdefend_dist * Robot::MAX_RADIUS);
        }
        else
        {
            target = Point(t.x, t.y + tdefend_dist * Robot::MAX_RADIUS);
        }
    }
    return target;
}

Point tdefender_block_enemy(World world, Point r, const unsigned index)
{
    Point dirToGoal;
    dirToGoal   = (world.field().friendly_goal() - r).norm();
    Point block = world.field().friendly_goal() -
                  (6 * (index + 1) * Robot::MAX_RADIUS * dirToGoal);
    if (index == 3)
    {
        block = world.field().friendly_goal() -
                (6 * (1.6) * Robot::MAX_RADIUS * dirToGoal);
    }
    return block;
}
}

bool AI::HL::STP::Evaluation::ball_in_friendly_crease(World world)
{
    bool bBallInCrease;
    Point ballPos = world.ball().position();

    bBallInCrease =
        ballPos.y < world.field().friendly_crease_pos_corner().y &&
        ballPos.y > world.field()
                        .friendly_crease_neg_corner()
                        .y  // is the ball within the goalie crease?
        && ballPos.x < (world.field().friendly_crease_pos_corner().x) &&
        ballPos.x > world.field().friendly_goal().x;

    return bBallInCrease;
}



Point AI::HL::STP::Evaluation::evaluateShallowAngleBlock(World world, Point position) { //TODO comment this
    Point negGoalPost = world.field().friendly_goalpost_neg();
    Point posGoalPost = world.field().friendly_goalpost_pos();

    Point dirToNegPost = (negGoalPost - position).norm();
    Point dirToPosPost = (posGoalPost - position).norm();

    Point negPostIntersect;
    Point posPostIntersect;

    Point blockPos;

    double criticalValue;
    double scaleFactorPos;
    double scaleFactorNeg;
    double defenseAreaIntersectLength;


    double phi;
    double theta;
    double x;

    if (position.y < 0) {
        criticalValue = world.field().friendly_crease_neg_corner().y + Robot::MAX_RADIUS;
        negPostIntersect.y = criticalValue;
    } else {
        criticalValue = world.field().friendly_crease_pos_corner().y - Robot::MAX_RADIUS;
        posPostIntersect.y = criticalValue;
    }

    scaleFactorPos = (position.y - criticalValue)/dirToPosPost.y;
    scaleFactorNeg = (position.y  - criticalValue)/dirToNegPost.y;


    negPostIntersect.x = position.x - (scaleFactorNeg * dirToNegPost.x);
    posPostIntersect.x = position.x - (scaleFactorPos * dirToPosPost.x);

    defenseAreaIntersectLength = posPostIntersect.x - negPostIntersect.x;

    if ( fabs(defenseAreaIntersectLength) >= Robot::MAX_RADIUS*2) {
        blockPos.x = negPostIntersect.x + defenseAreaIntersectLength/2;
        blockPos.y = criticalValue;
    }
    else {
        phi = pow((dirToNegPost.x - dirToPosPost.x),2);
        theta = pow((dirToNegPost.y - dirToPosPost.y),2); // calculate the position in the net that has the same size gap from the goalpost vectors at the diameter of the goalie
        x = pow(Robot::MAX_RADIUS*1.7, 2);
        x /= (phi+theta);

        x = sqrt(x);

        if (position.y < 0 ) {
            blockPos = position + (world.field().friendly_goal() - position).norm()*x;
        }
        else {
            blockPos.x = position.x + (world.field().friendly_goal() - position).norm().x*x;
            blockPos.y = position.y + (world.field().friendly_goal() - position).norm().y*x;
            // LOGF_INFO("block x: %1, block y: %2", blockPos.x, blockPos.y);
        }
        if (blockPos.x < world.field().friendly_goal().x+0.05) {
            blockPos.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;

            if (position.y < 0) {
                blockPos.y = world.field().friendly_goalpost_neg().y - Robot::MAX_RADIUS;
            } else {
                blockPos.y = world.field().friendly_goalpost_pos().y + Robot::MAX_RADIUS;
            }
        } else if (blockPos.x > world.field().friendly_crease_pos_corner().x) {
            blockPos.x = world.field().friendly_crease_pos_corner().x;
        }
    }

    //LOGF_INFO("blockkk x: %1, block y: %2", blockPos.x, blockPos.y);
    return blockPos;

}

std::vector<Point> AI::HL::STP::Evaluation::evaluate_shots(World world, Point position) {

    Point dirToNegPost = (world.field().friendly_goalpost_neg() - position).norm();
    Point dirToPosPost = (world.field().friendly_goalpost_pos() - position).norm();
    Point dirToCenterGoal = (world.field().friendly_goal() - position).norm();

    Point negPostIntersect;
    Point posPostIntersect;

    Point blockPos;

    double criticalValue = 2 * Robot::MAX_RADIUS + 0.04;
    double scaleFactorPos;
    double scaleFactorNeg;
    double defenseAreaIntersectLength;
    double scaleFactor;
    Angle coneAngle = (dirToNegPost.orientation() - dirToPosPost.orientation());

    Point defensePoint1;
    Point defensePoint2;
    Point defensePointGoalie;

    bool bPoint1InCrease;
    bool bPoint2InCrease;
    bool bGoalieInCrease;

    std::vector<Point> defensePoints;

    scaleFactor = criticalValue / coneAngle.sin();

    std::vector<Point> block;

    defensePoint1 = position + dirToNegPost * scaleFactor;
    defensePoint2 = position + dirToPosPost * scaleFactor;


    bPoint1InCrease =
            defensePoint1.y < world.field().friendly_crease_pos_corner().y &&
            defensePoint1.y > world.field()
                    .friendly_crease_neg_corner()
                    .y  // is the ball within the goalie crease?
            && defensePoint1.x < (world.field().friendly_crease_pos_corner().x) &&
            defensePoint1.x > world.field().friendly_goal().x;

    bPoint2InCrease =
            defensePoint2.y < world.field().friendly_crease_pos_corner().y &&
            defensePoint2.y > world.field()
                    .friendly_crease_neg_corner()
                    .y  // is the ball within the goalie crease?
            && defensePoint2.x < (world.field().friendly_crease_pos_corner().x) &&
            defensePoint2.x > world.field().friendly_goal().x;

    defensePoints = Evaluation::positionFriendlyCreaseIntersect(world, world.ball().position());

    if (bPoint1InCrease || defensePoint1.x < -world.field().length()/2) {
        defensePoint1 = defensePoints[1];
    }

    if (bPoint2InCrease || defensePoint2.x < -world.field().length()/2) {
        defensePoint2 = defensePoints[2];
    }

    defensePointGoalie = positionFriendlyCreaseIntersect(world, world.ball().position())[0];
//    LOGF_INFO("neg..x:%1, y:%2", defensePoint1.x, defensePoint1.y);
//    LOGF_INFO("pos..x:%1, y:%2", defensePoint2.x, defensePoint2.y);

    block.push_back(defensePoint1);
    block.push_back(defensePoint2);
    block.push_back(defensePointGoalie);

    return block;

}

std::vector<Point> AI::HL::STP::Evaluation::positionFriendlyCreaseIntersect(World world, Point position) {
    Angle dirToGoal = (position - world.field().friendly_goal() ).orientation();
    double criticalValue;
    double scaleFactor;
    Point Intersect;
    Point defender1Intersect;
    Point defender2Intersect;
    Point endLineIntersect;

    std::vector<Point> intersects;
//
//    if (dirToGoal.orientation().to_degrees() > 90 && dirToGoal.orientation().to_degrees() < 135) {
//        criticalValue = world.field().friendly_crease_neg_corner().y;
//
//        scaleFactor   = (criticalValue - position.y)/dirToGoal.y;
//        intersect.y   = criticalValue;
//        intersect.x   = position.x - scaleFactor * dirToGoal.y;
//        LOG_INFO("CASE1");
//
//        defender1Intersect = intersect;
//        defender2Intersect = intersect;
//
//        defender1Intersect.x += Robot::MAX_RADIUS;
//        defender2Intersect.x -= Robot::MAX_RADIUS;
//
//    } else if (dirToGoal.orientation().to_degrees() <= 135 && dirToGoal.orientation().to_degrees() > -135) {
//        criticalValue = world.field().friendly_crease_neg_corner().x;
//
//        scaleFactor   = (criticalValue - position.x)/dirToGoal.x;
//        intersect.x   = criticalValue;
//        intersect.y   = position.y - scaleFactor * dirToGoal.y;
//
//        defender1Intersect = intersect;
//        defender2Intersect = intersect;
//        LOG_INFO("CASE2");
//
//        defender1Intersect.y += Robot::MAX_RADIUS;
//        defender2Intersect.y -= Robot::MAX_RADIUS;
//
//    } else {
//        criticalValue = world.field().friendly_crease_pos_corner().y;
//        LOG_INFO("CASE3");
//        scaleFactor   = (criticalValue - position.y)/dirToGoal.y;
//        intersect.y   = criticalValue;
//        intersect.x   = position.x - scaleFactor * dirToGoal.x;
//
//        defender1Intersect = intersect;
//        defender2Intersect = intersect;
//
//        defender1Intersect.x -= Robot::MAX_RADIUS;
//        defender2Intersect.x += Robot::MAX_RADIUS;
//    }

    if (dirToGoal < Angle::of_degrees(-45))
    {
        criticalValue = world.field().friendly_crease_neg_corner().y;
        // criticalValue = goaliePos.crit + scaleFactor*trig(goalieangle)
        // scalefactor = (criticalValue - goaliePos.crit) / trig(angle)
        scaleFactor = (criticalValue - world.field().friendly_goal().y) / dirToGoal.sin();
        Intersect.y = criticalValue;
        Intersect.x = world.field().friendly_goal().x + scaleFactor * dirToGoal.cos();

        defender1Intersect = Intersect;
        defender2Intersect = Intersect;

        defender1Intersect.x += Robot::MAX_RADIUS;
        defender2Intersect.x -= Robot::MAX_RADIUS;

        if (defender1Intersect.x <= world.field().friendly_goal().x) {
            defender1Intersect.x = defender2Intersect.x + Robot::MAX_RADIUS;
        }
        defender1Intersect.y -= 2*Robot::MAX_RADIUS;
        defender2Intersect.y -= 2*Robot::MAX_RADIUS;
    }
    else if (dirToGoal > Angle::of_degrees(45))
    {
        criticalValue = world.field().friendly_crease_pos_corner().y;
        scaleFactor   = (criticalValue - world.field().friendly_goal().y) / dirToGoal.sin();
        Intersect.y   = criticalValue;
        Intersect.x   = world.field().friendly_goal().x + scaleFactor * dirToGoal.cos();

        defender1Intersect = Intersect;
        defender2Intersect = Intersect;

        defender1Intersect.x -= Robot::MAX_RADIUS;
        defender2Intersect.x += Robot::MAX_RADIUS;

        defender1Intersect.y += 2*Robot::MAX_RADIUS;
        defender2Intersect.y += 2*Robot::MAX_RADIUS;

        if (defender2Intersect.x <= world.field().friendly_goal().x) {
            defender2Intersect.x = defender1Intersect.x + Robot::MAX_RADIUS;
        }

    }
    else
    {
        criticalValue = world.field().friendly_crease_pos_corner().x;
        scaleFactor   = (criticalValue - world.field().friendly_goal().x) / dirToGoal.cos();
        Intersect.x   = criticalValue;
        Intersect.y   = world.field().friendly_goal().y + scaleFactor * dirToGoal.sin();

        defender1Intersect = Intersect;
        defender2Intersect = Intersect;

        defender1Intersect.y += Robot::MAX_RADIUS;
        defender2Intersect.y -= Robot::MAX_RADIUS;

        defender1Intersect.x += 2*Robot::MAX_RADIUS;
        defender2Intersect.x += 2*Robot::MAX_RADIUS;
    }

    Point goalieInter = Intersect + 0.5*(Intersect - world.ball().position()).norm();

    intersects.push_back(Intersect);
    intersects.push_back(defender1Intersect);
    intersects.push_back(defender2Intersect);
    intersects.push_back(goalieInter);
    return intersects;

}

Point endline_intersection(World world, Point position) {
    double criticalValue;
    double scaleFactor;

    Point direction;
    Point blockPos;

    direction = (world.field().friendly_goal() - position).norm();

    scaleFactor = (world.field().friendly_goal().x - position.x);

    blockPos.x = criticalValue + Robot::MAX_RADIUS;
    blockPos.y = position.y + scaleFactor*direction.y;

    return blockPos;

}
void AI::HL::STP::Evaluation::tick_defense(World world)
{
    waypoints = compute(world);
}

const std::array<Point, MAX_DEFENDERS + 1>
AI::HL::STP::Evaluation::evaluate_defense()
{
    return waypoints;
}

bool AI::HL::STP::Evaluation::enemy_break_defense_duo(
    World world, const Robot enemy)
{
    std::vector<Point> obstacles;
    obstacles.push_back(waypoints[0]);
    obstacles.push_back(waypoints[1]);

    return calc_enemy_best_shot_goal(world.field(), obstacles, enemy.position())
               .second > enemy_shoot_accuracy;
}

Point AI::HL::STP::Evaluation::evaluate_tdefense(
    World world, const unsigned index)
{
    Point target;
    if (world.enemy_team().size() > index - 1)
    {
        std::vector<Robot> enemies = Evaluation::enemies_by_grab_ball_dist();
        Point r                    = enemies[index - 1].position();
        if (r.x < world.field().centre_circle_radius())
        {
            target = tdefender_block_enemy(world, r, index);
        }
        else
        {
            target = tdefender_block_ball(world, index);
        }
    }
    else
    {
        target = tdefender_block_ball(world, index);
    }

    if (target.x < world.field().friendly_goal().x + Robot::MAX_RADIUS)
    {  // avoid going inside the goal
        target.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
    }
    return target;
}

Point AI::HL::STP::Evaluation::evaluate_tdefense_line(
    World world, const Point p1, const Point p2, const double dist_min,
    const double dist_max)
{
    Point ball = world.ball().position(), target = (p1 + p2) / 2;
    double diff = (ball - target).len();
    if (diff > dist_max || diff < dist_min)
        return ball;
    return target;
}

Point AI::HL::STP::Evaluation::evaluate_shadow_enemy_point(
    World world, const int closest_enemy_index)
{
    Point enemy =
        Enemy::closest_ball(world, closest_enemy_index)->evaluate().position();
    Point ball        = world.ball().position();
    Point destination = ball - enemy;
    Point rectangle[4];
    Point line_segment[2];

    line_segment[0] = enemy;
    line_segment[1] = world.field().friendly_goal();

    rectangle[0] = Point(
        world.field().friendly_goal().x + world.field().goal_width() / 2,
        world.field().friendly_goal().y);
    rectangle[1] = Point(
        world.field().friendly_goal().x - world.field().goal_width() / 2,
        world.field().friendly_goal().y);
    rectangle[2] = enemy;
    rectangle[3] = enemy;

    destination = destination.norm() *
                  (AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS);
    destination = ball - destination;
    return destination;
}
