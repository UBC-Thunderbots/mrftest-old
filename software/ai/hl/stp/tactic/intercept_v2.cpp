#include <algorithm>

#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/stop.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/intercept.h"
#include "ai/hl/stp/tactic/intercept_v2.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "geom/util.h"

using namespace std;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace Geom;
using namespace AI::HL::Util;
namespace Action     = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace
{
class Intercept_v2 final : public Tactic
{
   public:
    explicit Intercept_v2(World world) : Tactic(world)
    {
    }

   private:
    void execute(caller_t& ca) override;
    Player select(const std::set<Player>& players) const override;
    Robot get_closest_enemy(World world, Point target);
    double get_intercept_time(Point initial_pos, Point target);

    double R_Radius         = Robot::MAX_RADIUS;
    double delta_t          = 0.01;
    double A_MAX            = 3.0;
    double V_MAX            = 2.0;
    double PI               = 3.141592653589793238462;
    double ballVelThreshold = 0.001;
    Rect fieldBounds        = Rect(
        world.field().friendly_corner_pos(), world.field().enemy_corner_neg());

    Glib::ustring description() const override
    {
        return u8"Intercept ball";
    }
};

Player Intercept_v2::select(const std::set<Player>& players) const
{
    // return select_baller(world, players, player());
    return *(players.begin());
}

void Intercept_v2::execute(caller_t& ca)
{
    while (true)
    {
        // if the interceptor already has the ball, stay there
        if (player().has_ball())
        {
            Action::stop(ca, world, player());
            yield(ca);
            continue;
        }

        // FOR TESTING. DO NOT MOVE UNLESS THE BALL DOES
        if (world.ball().velocity().len() < ballVelThreshold)
        {
            Action::stop(ca, world, player());
            yield(ca);
            continue;
        }

        // If the ball is barely moving, get behind it in a position to shoot
        if (world.ball().velocity().len() < ballVelThreshold)
        {
            // USE CATCH HERE
            // Action::move(world, player, world.ball().position() +
            // (world.ball().position() -
            // world.field().enemy_goal()).norm(R_Radius));
            Action::catch_ball(ca, world, player(), world.ball().position());
            yield(ca);
            continue;
        }

        // IF THE ROBOT IS IN THE BALLS PATH, AND THE BALL IS CLOSE, STOP
        if ((player().position() -
             closest_lineseg_point(
                 player().position(), world.ball().position(),
                 world.ball().position() + world.ball().velocity().norm(10.0)))
                    .len() < R_Radius / 5.0 &&
            (world.ball().position() - player().position()).len() <
                R_Radius * 4)
        {
            Action::stop(ca, world, player());
            yield(ca);
            continue;
        }

        // set up variables
        double time       = 0.0;
        double best_score = -1000.0;
        double score      = 0.0;
        Point best_point =
            world.ball().position() +
            (player().position() - world.ball().position())
                .norm(R_Radius);  // defaults to the balls current position
        Point new_best_point =
            world.ball().position() +
            (player().position() - world.ball().position())
                .norm(R_Radius);  // defaults to the balls current position

        Point predicted_ball_pos = world.ball().position();
        Point predicted_intercept_pos;  // the location the robot should be at
                                        // to intercept. 1 radius in front of
                                        // the ball
        double time_to_intercept;  // time it will take for interceptor to reach
                                   // intercept position
        Robot closest_enemy;
        double enemy_time_to_intercept;  // how long it will take for closest
                                         // enemy to reach intercept position

        do
        {
            predicted_ball_pos = world.ball().position(time);
            predicted_intercept_pos =
                predicted_ball_pos +
                world.ball().velocity().norm(R_Radius * 1.25);
            time_to_intercept = get_intercept_time(
                player().position(), predicted_intercept_pos);
            closest_enemy = get_closest_enemy(world, predicted_intercept_pos);
            enemy_time_to_intercept = get_intercept_time(
                closest_enemy.position(), predicted_intercept_pos);

            // if the interceptor can intercept in time, evaluate the point.
            // Otherwise, skip to the next point.
            if (time_to_intercept < time)
            {
                // if the enemy can intercept first, do something special?
                if (enemy_time_to_intercept < time_to_intercept)
                {
                    return;
                }
                else
                {
                    // Evaluate the point, return score
                    score = Evaluation::getBestIntercept(
                        world, player(), predicted_intercept_pos, time);

                    // Keep the highest-scoring point
                    if (score > best_score)
                    {
                        best_score     = score;
                        new_best_point = predicted_intercept_pos;
                    }
                }
            }
            else
            {
                new_best_point = line_rect_intersect(
                                     fieldBounds, world.ball().position(),
                                     world.ball().position() +
                                         world.ball().velocity().norm(10))
                                     .front();
            }

            time += delta_t;
            yield(ca);
        } while (time <= 5 &&
                 fieldBounds.point_inside(predicted_ball_pos) == true);

        // Check that the new best intercept point is sufficiently far from the
        // old one
        if ((new_best_point - best_point).len() > R_Radius / 2.0)
        {
            best_point = new_best_point;
        }

        Action::move_careful(ca, world, player(), best_point);
        yield(ca);
    }
}

double Intercept_v2::get_intercept_time(Point initial_pos, Point target)
{
    Point target_dir =
        target -
        initial_pos;  // direction interceptor must go to reach target point
    double dist = target_dir.len();
    return dist / (V_MAX * 0.95) + 0.2;
}

// Returns the robot object of the closest enemy to the given target
Robot Intercept_v2::get_closest_enemy(World world, Point target)
{
    EnemyTeam enemy_team = world.enemy_team();
    Robot enemy;
    Robot closest_enemy;
    double dist;
    double shortest_dist = 1000.0;

    for (unsigned int i = 0; i < enemy_team.size(); i++)
    {  // check if goes out of bounds with <=?
        enemy = enemy_team[i];
        dist  = (enemy.position() - target).len();

        if (dist < shortest_dist)
        {
            shortest_dist = dist;
            closest_enemy = enemy;
        }
    }
    return closest_enemy;
}
}

Tactic::Ptr AI::HL::STP::Tactic::intercept_v2(World world)
{
    Tactic::Ptr p(new Intercept_v2(world));
    return p;
}
