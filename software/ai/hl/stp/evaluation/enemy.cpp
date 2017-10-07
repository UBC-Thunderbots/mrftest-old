#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"

#include <algorithm>

using namespace AI::HL::STP;
namespace Util = AI::HL::Util;

DegreeParam AI::HL::STP::Evaluation::enemy_shoot_accuracy(
    u8"Enemy shoot accuracy (degrees)", u8"AI/HL/STP/enemy", 1.0, 0.0, 90.0);

namespace
{
struct EnemyRisk
{
    Robot enemy;
    double risk;
    EnemyRisk(Robot r, double x) : enemy(r), risk(x)
    {
    }
};
struct by_risk
{
    bool operator()(EnemyRisk const &a, EnemyRisk const &b)
    {
        return a.risk > b.risk;
    }
};
}

bool AI::HL::STP::Evaluation::enemy_can_shoot_goal(World world, Robot enemy)
{
    return calc_enemy_best_shot_goal(world, enemy).second >
           enemy_shoot_accuracy;
}

/*
   std::pair<Point, double>
   AI::HL::STP::Evaluation::calc_enemy_best_shot_target(World world, const Point
   &target_pos, const Robot enemy, const double radius) {
    std::vector<Point> obstacles;
    const FriendlyTeam friendly = world.friendly_team();
    for (std::size_t i = 0; i < friendly.size(); ++i) {
        obstacles.push_back(friendly.get(i).position());
    }
    EnemyTeam enemies = world.enemy_team();
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        const Robot erob = enemies.get(i);
        if (erob == enemy) {
            continue;
        }
        obstacles.push_back(erob.position());
    }

    return calc_enemy_best_shot_target(target_pos, obstacles, enemy.position(),
   radius);
   }
 */

/*
   std::pair<Point, double>
   AI::HL::STP::Evaluation::calc_enemy_best_shot_target(const Point &target_pos,
   const std::vector<Point> &obstacles, const Point &p, const double radius) {

   #warning due to an old HACK, angle_sweep_circle only works on positive side,
   so this is a hack because of a hack

    std::vector<Point> obs_rev = obstacles;
    for (std::size_t i = 0; i < obs_rev.size(); ++i) {
        obs_rev[i].x *= -1;
    }

    Point target_pos_rev = target_pos;
    target_pos_rev.x *= -1;

    Point p_rev = p;
    p_rev.x *= -1;

    auto ret = AI::HL::Util::calc_best_shot_target(target_pos_rev, obs_rev,
   p_rev, radius);
    ret.first.x *= -1;

    return ret;
   }
 */

std::pair<Point, Angle> Evaluation::calc_enemy_best_shot_goal(
    const Field &f, const std::vector<Point> &obstacles, const Point &p,
    const double radius)
{
#warning due to an old HACK, angle_sweep_circle only works on positive side, so this is a hack because of a hack

    std::vector<Point> obs_rev = obstacles;
    for (std::size_t i = 0; i < obs_rev.size(); ++i)
    {
        obs_rev[i].x *= -1;
    }

    const Point p1 = Point(f.length() / 2.0, -f.goal_width() / 2.0);
    const Point p2 = Point(f.length() / 2.0, f.goal_width() / 2.0);

    auto ret = angle_sweep_circles(
        Point(-p.x, p.y), p1, p2, obs_rev, radius * Robot::MAX_RADIUS);
    ret.first.x *= -1;
    return ret;

    /*
       const Point p1 = Point(-f.length() / 2.0, f.goal_width() / 2.0);
       const Point p2 = Point(-f.length() / 2.0, -f.goal_width() / 2.0);
       return angle_sweep_circles(p, p1, p2, obstacles, radius *
       Robot::MAX_RADIUS);
     */
}

std::pair<Point, Angle> Evaluation::calc_enemy_best_shot_goal(
    World world, const Robot enemy, const double radius)
{
    std::vector<Point> obstacles;
    for (const Player i : world.friendly_team())
    {
        obstacles.push_back(i.position());
    }
    for (const Robot erob : world.enemy_team())
    {
        if (erob == enemy)
        {
            continue;
        }
        obstacles.push_back(erob.position());
    }
    return calc_enemy_best_shot_goal(
        world.field(), obstacles, enemy.position(), radius);
}

std::vector<Robot> AI::HL::STP::Evaluation::get_passees(
    World world, Robot robot)
{
    // don't count this robot
    std::vector<Robot> passees;
    for (const Robot i : world.enemy_team())
    {
        if (robot == i)
        {
            continue;
        }
        if (enemy_can_pass(world, robot, i))
        {
            passees.push_back(i);
        }
    }
    return passees;
}

int AI::HL::STP::Evaluation::calc_enemy_pass(World world, const Robot robot)
{
    EnemyTeam enemy = world.enemy_team();

    auto threats = calc_enemy_threat(world);

    for (std::size_t i = 0; i < enemy.size(); ++i)
    {
        if (enemy[i] != robot)
        {
            continue;
        }

        return threats[i].passes_goal;
    }

    return 5;

    /*
       EnemyTeam enemy = world.enemy_team();

       // can shoot directly
       if (enemy_can_shoot_goal(world, robot)) {
       return 0;
       }

       int passes = 5;

       for (size_t i = 0; i < enemy.size(); ++i) {
       Robot passee = enemy.get(i);
       if (passee == robot) continue;
       if (!enemy_can_pass(world, robot, passee)) continue;
       if (enemy_can_shoot_goal(world, passee)) {
       passes = std::min(passes, 1);
       continue;
       }

       for (std::size_t j = 0; j < enemy.size(); ++j) {
       Robot next_passee = enemy.get(j);
       if (next_passee == robot) continue;
       if (next_passee == passee) continue;
       if (!enemy_can_pass(world, passee, next_passee)) continue;
       if (!enemy_can_shoot_goal(world, next_passee)) continue;
       passes = std::min(passes, 2);
       }
       }

       return passes;
     */
}

std::vector<Evaluation::Threat> AI::HL::STP::Evaluation::calc_enemy_threat(
    World world)
{
    EnemyTeam enemy = world.enemy_team();

    Robot enemy_baller = calc_enemy_baller(world);

    std::vector<Evaluation::Threat> threats(enemy.size());
    for (size_t i = 0; i < enemy.size(); ++i)
    {
        threats[i].goal_angle =
            calc_enemy_best_shot_goal(world, enemy[i]).second;
        threats[i].can_shoot_goal = enemy_can_shoot_goal(world, enemy[i]);
        threats[i].passes_reach   = 5;
        threats[i].passes_goal    = 5;
        threats[i].robot          = enemy[i];

        if (enemy[i] == enemy_baller)
        {
            threats[i].passes_reach = 0;
        }

        if (possess_ball(world, enemy[i]))
        {
            threats[i].passes_reach = 0;
        }
        if (threats[i].can_shoot_goal)
        {
            threats[i].passes_goal = 0;
        }
    }

    // all-pairs shortest paths
    for (size_t k = 0; k < enemy.size(); ++k)
    {
        for (size_t i = 0; i < enemy.size(); ++i)
        {
            for (size_t j = 0; j < enemy.size(); ++j)
            {
                if (i == j)
                {
                    continue;
                }
                if (!enemy_can_pass(world, enemy[i], enemy[j]))
                {
                    continue;
                }

                // if can pass from i to j

                if (threats[j].passes_reach > threats[i].passes_reach + 1)
                {
                    threats[j].passes_reach = threats[i].passes_reach + 1;
                    threats[j].passer       = enemy[i];
                }

                if (threats[i].passes_goal > threats[j].passes_goal + 1)
                {
                    threats[i].passes_goal = threats[j].passes_goal + 1;
                    threats[i].passee      = enemy[j];
                }
            }
        }
    }

    return threats;
}

double AI::HL::STP::Evaluation::enemy_passee_risk(
    World world, std::vector<Point> obstacles, Robot enemy_passee,
    Robot enemy_baller, double future_block_time)
{
    bool goalie_top = true;
    std::pair<Point, Point> friendly_goal_posts =
        world.field().friendly_goal_boundary();
    // const Point goal_side = goalie_top ? Point(-field.length() / 2,
    // field.goal_width() / 2) : Point(-field.length() / 2, -field.goal_width()
    // / 2);
    // const Point goal_opp = goalie_top ? Point(-field.length() / 2,
    // -field.goal_width() / 2) : Point(-field.length() / 2, field.goal_width()
    // / 2);

    Point goal_side = friendly_goal_posts.first;
    Point goal_opp  = friendly_goal_posts.second;

    std::pair<Point, Angle> risk_zone = angle_sweep_circles(
        enemy_passee.position(future_block_time), goal_opp, goal_side,
        obstacles, Robot::MAX_RADIUS);
    Angle goal_angle = risk_zone.second;

    double risk = goal_angle.to_degrees();
    return (goal_angle.to_degrees());
}

std::vector<Robot> AI::HL::STP::Evaluation::rank_enemy_passee_risk(
    World world, std::vector<Point> obstacles, double future_block_time)
{
    EnemyTeam enemy_team = world.enemy_team();
    Robot enemy_baller   = calc_enemy_baller(world);

    std::vector<EnemyRisk> enemy_risks;

    for (std::size_t i = 0; i < enemy_team.size(); ++i)
    {
        if (enemy_team[i].pattern() != enemy_baller.pattern())
        {
            double risk = enemy_passee_risk(
                world, obstacles, enemy_team[i], enemy_baller,
                future_block_time);
            enemy_risks.push_back(EnemyRisk(enemy_team[i], risk));
        }
    }

    std::sort(enemy_risks.begin(), enemy_risks.end(), by_risk());
    std::vector<Robot> ranked_enemies;

    for (auto enemy_risk : enemy_risks)
    {
        ranked_enemies.push_back(enemy_risk.enemy);
    }
    return (ranked_enemies);
}
