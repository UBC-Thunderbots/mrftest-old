//
// Created by evan on 19/05/18.
//

#include "defend.h"

#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/shoot.h"

#include "ai/hl/stp/enemy.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/shoot.h"

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "math.h"
#include "util/dprint.h"

#include <algorithm>

#include <cassert>

#include "defend.h"

#include "ai/hl/stp/gradient_approach/PassInfo.h"
#include "ai/hl/stp/world.h"

#define GOALIE_DEFEND_RADIUS 1
#define PASSING_SPEED_CUTOFF 0.4
#define PASSING_DISTANCE_THRESHHOLD 0.05
#define PASSING_YPRIME_THRESHHOLD 0.30
#define PASSING_CONE_ANGLE (3.14 / 6)  // 30 degrees
#define GOALIE_SHADOW_OFFSET 0.3
#define DEGREE_TO_RAD 0.0174533
#define RAD_TO_DEGREE (1 / DEGREE_TO_RAD)
#define GOAL_LENGTH 1
#define DEFENDER_SWITCH_ANGLE                                                  \
    45  // the angle at which the goalie/defender has to switch logic based on
        // the rectuangular defense area
#define DEFENDER_INTERSECT_OFFSET 2.5
#define DEFENDER_CREASE_OFFSET 1.3

#define REJECT_BALL_PIVOT_OFFSET (0.05)
#define REJECT_BALL_VELOCITY_THRESHHOLD 0.04
#define REJECT_BALL_PIVOT_ANGLE_TOL 15
#define REJECT_BALL_PIVOT_POSITION_TOL 0.02

#define BLOCK_REJECT_DISTANCE_THRESHHOLD 1
#define BLOCK_BALL_LOCATION_OFFSET (3 * Robot::MAX_RADIUS)
#define BLOCK_BALL_GOAL_EXTENSION_FACTOR 1.2
#define GOALIE_CHIP_POWER 6
#define GOALIE_CHIP_TOLERENCE 10
#define MIN_SHOT_SPEED 0.8
#define LOWPROFILE_SHOT (Angle::of_degrees(25).cos())

#define PREPARING_SHOT_ORIENTATION_TRESHHOLD 50  // angle in degrees
#define PREPARING_SHOT_ROBOTSPEED_THRESHHOLD                                   \
    0.35  // robot must not be moving fast to line up a shot
#define PREPARING_SHOT_BALLSPEED_THRESHHOLD                                    \
    0.1  // ball must be moving slow when preparing for a shot
#define PREPARING_SHOT_PROXIMITY_THRESHHOLD                                    \
    0.50  // robot must be close to the ball if it is preparing to shoot
#define DEFENDER_CHIP_THRESHHOLD 0.4
#define DEFENDER_CHIP_POWER 3
#define CLOSE_TO_FRIENDLY_ENDLINE                                              \
    (world.field().friendly_goal().x + Robot::MAX_RADIUS)
#define CLOSE_TO_FRIENDLY_GOALPOST                                             \
    (world.field().friendly_goalpost_pos().y + 2 * Robot::MAX_RADIUS)

#define BALLER_BLOCKER_DISTANCE 0.5
#define CHIP true

#define JITTER_THRESHHOLD 0.01


// running reject ball while ball is moving fast
// running shadow ball too much (sucks)
// corner of defense area shots
// convert eval to be based off a point (for use with ball vs robot)
// goalie abs value shot problem

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace Geom;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action     = AI::HL::STP::Action;

// TODO: Make defenders not switch tactics rapidly
//       Make defenders block pass-shots on net
//       Make defenders move active in attacking the ball
//       Make a defender class that goes after the ball
//       Move all relevant functions to defense evaluation file
//       Add shot preparation logic to goalie

// goalie doesn't chase ball out of net
// pass prediciton
// shot prediction
// add parabola scaling for goalie distance out of net (depending on ball
// position on the field)

namespace
{

Point goalieStatePos;
Angle goalieStateAngle;


bool bGoalieAssistantActiveChip = false;

class GoalieAssistant1 final : public Tactic
{
   public:
    explicit GoalieAssistant1(World world) : Tactic(world)
    {
    }

   private:

    Point defenderLeftState;

    void shadowBall(World world, caller_t& ca);
    void blockShot(World world, caller_t& ca);

    void execute(caller_t& ca);
    Player select(const std::set<Player>& players) const override;
    Glib::ustring description() const override
    {
        return u8"Goalie Assistant 1";
    }
};

class GoalieAssistant2 final : public Tactic
{
   public:
    explicit GoalieAssistant2(World world) : Tactic(world)
    {
    }

   private:

    Point defenderRightState;

    void shadowBall(World world, caller_t& ca);
    void blockShot(World world, caller_t& ca);
    void execute(caller_t& ca);
    Player select(const std::set<Player>& players) const override;
    Glib::ustring description() const override
    {
        return u8"Goalie Assistant 2";
    }
};

class BallerBlocker final : public Tactic
{
   public:
    explicit BallerBlocker(World world) : Tactic(world)
    {
    }

   private:
    Point blockerDest;
    void execute(caller_t& ca);
    Player select(const std::set<Player>& players) const override;
    Glib::ustring description() const override
    {
        return u8"Baller Blocker";
    }
};

class Goalie final : public Tactic
{
   public:
    explicit Goalie(World world) : Tactic(world)
    {
    }  // goalie constructor
    static Point goalieCreaseIntersection(
        World world, Point goaliePos,
        Angle goalieOri);  // returns the point on the goal crease that the
                           // goalie is pointing at FIXME: add to defense
                           // evaluation

   private:
    void execute(
        caller_t& ca);  // mandatory execute function that runs the tactic
    Player select(const std::set<Player>& players) const override;
    Glib::ustring description() const override
    {  // mandatory function that describes the tactic
        return u8"Goalie";
    }

    void goaliePositionPreparingShot(World world, caller_t& ca);  // there is a
                                                                  // robot close
                                                                  // to the ball
                                                                  // preparing
                                                                  // to take a
                                                                  // shot
    Point goaliePositionMovingShot(
        World world);  // the ball is moving towards the endLine
    Point goaliePosition(
        World world,
        Point ballDirection);  // positions the goalie to block a shot on net
    Point pointNormalize(Point point);  // normalized the point parameter (used
                                        // as a unit vector of direction)
    Point endLineIntersection(World world, Point ballDirection);  // calculates
                                                                  // where a
                                                                  // moving ball
                                                                  // will
                                                                  // intersect
                                                                  // friendly
                                                                  // endline
    Point blockPass(World world, Point passDirection);  // calculates the
                                                        // optimal goalie
                                                        // position to block a
                                                        // pass
    Robot isEnemyPassing(World world);  // calculates the most likely enemy
                                        // robot to be receiving a pass
    Point transformToBallAxis(
        Point coordinate, Point direction);  // transforms a point on the field
                                             // to a coordinate system that has
                                             // a horizontal axis in the same
                                             // direction as velocity
    std::vector<Robot> potentialEnemyPassReceivers(World world);  // returns a
                                                                  // container
                                                                  // of
                                                                  // potential
                                                                  // enemy
                                                                  // robots than
                                                                  // can reveive
                                                                  // a pass
    bool shotOnNet(World world);  // returns true/false if the ball trajectory
                                  // is into the friendly net
    bool enemyPreparingShot(World world);  // returns true/false if it appears
                                           // like an enemy robot is preparing
                                           // to take a shot
    void blockShot(
        caller_t& ca, World world);  // moves goalie to block a shot on net
    std::vector<Robot> distanceToBallSort(
        Point ballPos, std::vector<Robot> roboList);  // returns a sorted
                                                      // roboList from closest
                                                      // to the ball to furthest
    void goalieMove(
        caller_t&, World world, Point dest,
        Angle orientation);  // removed jitter from goalie by only accepting
                             // changes in position greater than a threshhold
    void rejectBall(caller_t& ca, World world);  // if the ball is in the goalie
                                                 // crease, then chip it at
                                                 // enemy net
    void blockAimedShot(caller_t& ca, World world);  // move to the location
                                                     // pointed to by an enemy
                                                     // preparing to take a shot
    void shadowBall(World, caller_t& ca);  // Goalie shadows ball when it's on
                                           // the enemy side of the field
                                           // (goalie position proportional to
                                           // field width and goal width)
    double shadowBallScale(World world);   // obtain the parabola used to scale
                                           // the goalie defense position
    double goalieDefendPosScale(World world); // depending on the position of the ball on the field, scale how far the goalie is pulled out of the net

    inline double passConeXToY(double xPrime, double theta)
    {  // used to calculare the maximum y' value from the direction of the ball
        // to be considered within possible receiving distance
        return tan(theta) * xPrime;  // tan(0) = y'/x'
                                     // //returns the y' point corresponding to
                                     // the x' and theta parameters
    }
};


Player Goalie::select(const std::set<Player>& players) const
{  // mandatory select function that selects the goalie robot
    assert(false);
}

Player GoalieAssistant1::select(const std::set<Player>& players) const
{
    return *std::min_element(
            players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(defenderLeftState));
}

Player GoalieAssistant2::select(const std::set<Player>& players) const
{
    // return select_baller(world, players, player());
    return *std::min_element(
            players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(defenderRightState));
}

Player BallerBlocker::select(const std::set<Player>& players) const
{
    return *std::min_element(
            players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(Evaluation::calc_enemy_baller(world).position()));
}

void GoalieAssistant1::execute(caller_t& ca)
{
    Point defender1Dest;

    while (true)
    {
        double distToBall =
            (world.ball().position() - player().position()).len();

        if (!Evaluation::ball_in_friendly_crease(world) &&
            distToBall < DEFENDER_CHIP_THRESHHOLD &&
            bGoalieAssistantActiveChip == false)
        {
            bGoalieAssistantActiveChip = true;
            // get behind ball and shoot it
            Action::shoot_target(
                ca, world, player(), world.field().enemy_goal(),
                DEFENDER_CHIP_POWER);
            bGoalieAssistantActiveChip = false;
            yield(ca);
        }
        else if (goalieStatePos.x > world.field().friendly_crease_pos_corner().x) {

            defender1Dest = world.field().friendly_crease_pos_corner();
            Action::move(ca, world, player(), defender1Dest);
            yield(ca);
        }
        else
        {
//            defender1Dest = Goalie::goalieCreaseIntersection(
//                world, goalieStatePos, goalieStateAngle);
//
//            if (goalieStateAngle <=
//                Angle::of_degrees(-DEFENDER_SWITCH_ANGLE))  // is the goalie
//                                                            // pointing to the
//                                                            // lower horizontal
//                                                            // line of the
//                                                            // crease?
//            {
//                defender1Dest.y -= Robot::MAX_RADIUS * DEFENDER_CREASE_OFFSET;
//                defender1Dest.x -=
//                    Robot::MAX_RADIUS * DEFENDER_INTERSECT_OFFSET;
//                yield(ca);
//            }
//            else if (
//                goalieStateAngle >=
//                Angle::of_degrees(DEFENDER_SWITCH_ANGLE))  // if the goalie
//                                                           // pointing to the
//                                                           // upper horizontal
//                                                           // line of the
//                                                           // crease?
//            {
//                defender1Dest.y += Robot::MAX_RADIUS * DEFENDER_CREASE_OFFSET;
//                defender1Dest.x +=
//                    Robot::MAX_RADIUS * DEFENDER_INTERSECT_OFFSET;
//                yield(ca);
//            }
//            else
//            {
//                defender1Dest.x +=
//                    Robot::MAX_RADIUS *
//                    DEFENDER_CREASE_OFFSET;  // goalie pointing towards the
//                                             // center veritcal line of the
//                                             // crease
//                defender1Dest.y -=
//                    Robot::MAX_RADIUS * DEFENDER_INTERSECT_OFFSET;
//                yield(ca);
//            }
//
//            if (defender1Dest.x < world.field()
//                                          .friendly_crease_neg_endline()
//                                          .x +  // if the defender would positio
//                                                // itself past the endline, then
//                                                // stay at the endline
//                                      Robot::MAX_RADIUS)
//            {
//                defender1Dest.x =
//                    world.field().friendly_crease_neg_endline().x +
//                    Robot::MAX_RADIUS;
//                yield(ca);
//            }
            shadowBall(world, ca);
            defenderLeftState = defender1Dest;
       }

        yield(ca);
    }
}


void GoalieAssistant2::execute(caller_t& ca)
{
    Point defender2Dest;

    while (true)
    {
        double distToBall =
            (world.ball().position() - player().position()).len();

        if (!Evaluation::ball_in_friendly_crease(world) &&
            distToBall < DEFENDER_CHIP_THRESHHOLD &&
            bGoalieAssistantActiveChip == false)
        {
            bGoalieAssistantActiveChip = true;
            // get behind ball and shoot it
            Action::shoot_target(
                ca, world, player(), world.field().enemy_goal(),
                DEFENDER_CHIP_POWER);
            bGoalieAssistantActiveChip = false;
            yield(ca);
        }
        else if (goalieStatePos.x > world.field().friendly_crease_neg_corner().x) {

            defender2Dest = world.field().friendly_crease_pos_corner();
            Action::move(ca, world, player(), defender2Dest);
            yield(ca);
        }
        else
        {
//            defender2Dest = Goalie::goalieCreaseIntersection(
//                world, goalieStatePos, goalieStateAngle);
//
//            if (goalieStateAngle <
//                Angle::of_degrees(-DEFENDER_SWITCH_ANGLE))  // is the goalie
//                                                            // pointing to the
//                                                            // lower horizontal
//                                                            // line of the
//                                                            // crease?
//            {
//                defender2Dest.y -= Robot::MAX_RADIUS * DEFENDER_CREASE_OFFSET;
//                defender2Dest.x +=
//                    Robot::MAX_RADIUS * DEFENDER_INTERSECT_OFFSET;
//                yield(ca);
//            }
//            else if (
//                goalieStateAngle >
//                Angle::of_degrees(DEFENDER_SWITCH_ANGLE))  // if the goalie
//                                                           // pointing to the
//                                                           // upper horizontal
//                                                           // line of the
//                                                           // crease?
//            {
//                defender2Dest.y += Robot::MAX_RADIUS * DEFENDER_CREASE_OFFSET;
//                defender2Dest.x -=
//                    Robot::MAX_RADIUS * DEFENDER_INTERSECT_OFFSET;
//                yield(ca);
//            }
//            else  // goalie pointing towards the center vertical line of the
//                  // crease
//            {
//                defender2Dest.x += Robot::MAX_RADIUS * DEFENDER_CREASE_OFFSET;
//                defender2Dest.y +=
//                    Robot::MAX_RADIUS * DEFENDER_INTERSECT_OFFSET;
//                yield(ca);
//            }
//
//            if (defender2Dest.x <
//                world.field().friendly_crease_neg_endline().x +
//                    Robot::MAX_RADIUS)  // if desired defender postion is past
//                                        // the endline, then stay on the endline
//            {
//                defender2Dest.x =
//                    world.field().friendly_crease_neg_endline().x +
//                    Robot::MAX_RADIUS;
//                yield(ca);
//            }

            shadowBall(world, ca);
            defenderRightState = defender2Dest;
        }
        yield(ca);
    }
}

void BallerBlocker::execute(
    caller_t& ca)  // this defender has the purpose of blocking the enemy baller
{
    while (true) {
        Robot enemyBaller = Evaluation::calc_enemy_baller(world); // get the enemy baller


        Angle blockerOri;

        if (enemyBaller.orientation() < Angle::of_degrees(270) && enemyBaller.orientation() > Angle::of_degrees(90)) { // if the enemy baller is facing our side of the field, block it's direction
            blockerDest.x = enemyBaller.position().x + enemyBaller.orientation().cos() * BALLER_BLOCKER_DISTANCE;
            blockerDest.y = enemyBaller.position().y + enemyBaller.orientation().sin() * BALLER_BLOCKER_DISTANCE;
            blockerOri = Angle::of_degrees(180) - enemyBaller.orientation();
        }
        else {
            blockerDest.x = enemyBaller.position().x - enemyBaller.orientation().cos() * BALLER_BLOCKER_DISTANCE; // if the enemy baller is facing their side of the net, block in the oposite direction the baller is facing
            blockerDest.y = enemyBaller.position().y + enemyBaller.orientation().sin() * BALLER_BLOCKER_DISTANCE;
            blockerOri = -enemyBaller.orientation();
        }
        



        Action::move(ca, world, player(), blockerDest, blockerOri);
        yield(ca);
    }
}

void Goalie::execute(caller_t& ca)
{
    Point test;

    while (true)
    {
        // check if the ball is shot
        // check if the ball is in the crease
        // check if an enemy is close to the ball
        // check is there is a potential pass (move goalie/defenders to block)
        // shadow the ball

        if (world.ball().position().x < world.field().friendly_goal().x)
        {
            Action::move(
                ca, world, player(),
                player().position());  // if the ball is past our endline (out
                                       // of bounds/goal) don't do anything
        }
        else if (Goalie::shotOnNet(world))
        {
            Goalie::blockShot(ca, world);
            LOG_INFO("BLOCKING SHOT");
        }
        else if (enemyPreparingShot(world))
        {
            Goalie::blockAimedShot(ca, world);
            LOG_INFO("PREPARING SHOT");
        }
        else if (
            Evaluation::ball_in_friendly_crease(world))
        {
            Goalie::rejectBall(ca, world);
            LOG_INFO("REJECTING BALL");
        }
        else
        {
            Goalie::shadowBall(world, ca);
            LOG_INFO("SHADOWING BALL");
        }

        yield(ca);
    }
}

void GoalieAssistant1::shadowBall(World world, caller_t& ca) {
    Point blockPos = Evaluation::evaluate_shots(world, world.ball().position())[0];
    if (player().position().x >= -world.field().length()/0.6 && blockPos.x > 0) {
        blockPos.x = player().position().x;
    }
    Action::move(ca, world, player(), blockPos, (world.ball().position() - player().position()).orientation());
}

void GoalieAssistant2::shadowBall(World world, caller_t& ca) {
        Point blockPos = Evaluation::evaluate_shots(world, world.ball().position())[1];
        if (player().position().x >= -world.field().length()/0.6 && blockPos.x > 0) {
            blockPos.x = player().position().x;
        }
        defenderRightState = blockPos;
        Action::move(ca, world, player(), blockPos, (world.ball().position() - player().position()).orientation());
}

void Goalie::shadowBall(World world, caller_t& ca)
{
    Point ballPos = world.ball().position();  // get the position of the ball
    Point blockPos;

    blockPos = Evaluation::evaluate_shots(world, world.ball().position())[3];


    Point goaliePos;
    goaliePos.y = ballPos.y * world.field().goal_width();
    goaliePos.x = world.field().friendly_goal().x + GOALIE_SHADOW_OFFSET;
    Angle goalieOrientation =
        (world.ball().position() - player().position()).orientation();

//    if (world.ball().position().x <=
//        world.field().friendly_goal().x + world.field().defense_area_width())
//    {   // if the ball is further down the field than the top of the goalie
//        // crease
//        if (world.ball().position().y >
//            world.field().defense_area_stretch() / 2)
//        {  // if the ball is on the positive-y side of the field
//            goaliePos.y =
//                world.field().friendly_goalpost_pos().y - Robot::MAX_RADIUS;
//        }
//        else if (
//            world.ball().position().y <
//            -world.field().defense_area_stretch() / 2)
//        {  // if the ball is on the negative-y side of the field
//            goaliePos.y =
//                world.field().friendly_goalpost_neg().y + Robot::MAX_RADIUS;
//        }
//
//        if (world.ball().position().x <
//            world.field().friendly_goal().x +
//                world.field().defense_area_width() / 2)
//        {   // if the ball is closer to the endline than the top of the goalie
//            // crease, pull the goalie out further to defend the net better
//            goaliePos.x =
//                world.field().friendly_goal().x + 1.5 * Robot::MAX_RADIUS;
//        }
//    }
    if (blockPos.x < -world.field().length()/2) {
        blockPos.x = -world.field().length()/2;
    }
    if (blockPos.y >= world.field().friendly_goalpost_pos().y +0.15) {
        blockPos.y = world.field().friendly_goalpost_pos().y+0.15;
    }
    if (blockPos.y <= world.field().friendly_goalpost_neg().y -0.15) {
        blockPos.y = world.field().friendly_goalpost_neg().y-0.15;
    }
    goalieStatePos  = goaliePos;
    goalieStateAngle = goalieOrientation;
    Action::move(ca, world, player(), blockPos, goalieOrientation);
}

Point Goalie::goalieCreaseIntersection(
    World world, Point goaliePos, Angle goalieOri)
{
    Point Intersect;
    double criticalValue;  // this value depends on the position of the goalie
                           // (x or y value that determines the intersection
                           // point)
    double scaleFactor = 0;

    if (goalieOri < Angle::of_degrees(-DEFENDER_SWITCH_ANGLE))
    {
        criticalValue = world.field().friendly_crease_neg_corner().y;
        // criticalValue = goaliePos.crit + scaleFactor*trig(goalieangle)
        // scalefactor = (criticalValue - goaliePos.crit) / trig(angle)
        scaleFactor = (criticalValue - goaliePos.y) / goalieOri.sin();
        Intersect.y = criticalValue;
        Intersect.x = goaliePos.x + scaleFactor * goalieOri.cos();
    }
    else if (goalieOri > Angle::of_degrees(DEFENDER_SWITCH_ANGLE))
    {
        criticalValue = world.field().friendly_crease_pos_corner().y;
        scaleFactor   = (criticalValue - goaliePos.y) / goalieOri.sin();
        Intersect.y   = criticalValue;
        Intersect.x   = goaliePos.x + scaleFactor * goalieOri.cos();
    }
    else
    {
        criticalValue = world.field().friendly_crease_pos_corner().x;
        scaleFactor   = (criticalValue - goaliePos.x) / goalieOri.cos();
        Intersect.x   = criticalValue;
        Intersect.y   = goaliePos.y + scaleFactor * goalieOri.sin();
    }

    // intersection = goaliePos + x*direction
    return Intersect;
}

void Goalie::rejectBall(caller_t& ca, World world)
{
    if (Evaluation::ball_in_friendly_crease(world))
    {
        if (world.ball().position().x <= CLOSE_TO_FRIENDLY_ENDLINE &&
            (abs(world.ball().position().y) <=
             world.field().friendly_goalpost_pos().y +
                 CLOSE_TO_FRIENDLY_GOALPOST))
        {  // if the ball is close to the goalposts, the goalie must go around
           // the ball to chip it out (not enough space to move behind it
           // directly)

            // find some reference vector
            // calculate how much to turn
            // pivot robot behind ball
            // run shoot_target()
            Point pivotStartDest;

            pivotStartDest.y =
                world.ball().position().y;  // the reference robot starting
                                            // positon should be in-line with
                                            // the ball
            pivotStartDest.x = world.ball().position().x + Robot::MAX_RADIUS +
                               REJECT_BALL_PIVOT_OFFSET;

            while ((player().position() - pivotStartDest).len() >=
                   REJECT_BALL_PIVOT_POSITION_TOL && Evaluation::ball_in_friendly_crease(world))
            {  // move the goalie in-fropnt of the ball
                Action::move(ca, world, player(), pivotStartDest);
                LOG_INFO("MOVING");
                yield(ca);
            }
            // put pivot logic here
            while ( (player().orientation() <
                       Angle::of_degrees(-REJECT_BALL_PIVOT_ANGLE_TOL) ||
                   player().orientation() >
                       Angle::of_degrees(REJECT_BALL_PIVOT_ANGLE_TOL)) && Evaluation::ball_in_friendly_crease(world))
            {
                Action::pivot(
                    ca, world, player(), world.ball().position(),
                    Angle::of_degrees(-179),
                    Robot::MAX_RADIUS + REJECT_BALL_PIVOT_OFFSET);
                LOG_INFO("PIVOTING");
                yield(ca);
            }

            Action::shoot_target(
                ca, world, player(), world.field().enemy_goal(),
                GOALIE_CHIP_POWER, CHIP);
        }
        else
        {
            Action::shoot_target(
                ca, world, player(), world.field().enemy_goal(),
                GOALIE_CHIP_POWER, CHIP);
        }
    }
}

bool Goalie::shotOnNet(World world)
{
    Point ballPos = world.ball().position();
    Point ballDir = world.ball().velocity().norm();
    double criticalValue =
        world.field().friendly_goal().x;  // critical value for determining if
                                          // the ball will end up in the
                                          // friendly net
    double scaleFactor;  // factor that will be calculated using the critical x
                         // value (will then be used to scale the y direction)
    double yEndlineIntercept;  // holds the y coordinate that the ball passes
                               // the friendly endline

    if (world.ball().velocity().len() < MIN_SHOT_SPEED)
    {
        return false;
    }
    // project the ball velocity towards the net (critical value is x of goal)
    // criticalValue = ballPos.x + scaleFactor*ballDir.x
    scaleFactor = (criticalValue - ballPos.x) / ballDir.x;

    yEndlineIntercept = ballPos.y + scaleFactor * ballDir.y;
    bool bGoal =
        (yEndlineIntercept < world.field().friendly_goalpost_pos().y *
                                 BLOCK_BALL_GOAL_EXTENSION_FACTOR &&
         yEndlineIntercept > world.field().friendly_goalpost_neg().y *
                                 BLOCK_BALL_GOAL_EXTENSION_FACTOR);

    //LOGF_INFO("x:%1. y:%2", )
    if (bGoal)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void Goalie::blockShot(caller_t& ca, World world)
{
    // block shot needs a separate case for shots close to the endlines (move
    // goalie more towards the ball for a better block)
    Point ballPos = world.ball().position();
    Point ballDir = world.ball().velocity().norm();
    double criticalValue =
        world.field().friendly_goal().x;  // critical value for determining if
                                          // the ball will end up in the
                                          // friendly net
    double scaleFactor;  // factor that will be calculated using the critical x
                         // value (will then be used to scale the y direction)
    Angle blockOri;      // angle to block the ball at (facing ball direction)
    Point blockLocation = world.field().friendly_goal();

    // project the ball velocity towards the net (critical value is x of goal)
    // criticalValue = ballPos.x + scaleFactor*ballDir.x
    scaleFactor = (criticalValue - ballPos.x) / ballDir.x;

    blockLocation.y = ballPos.y + scaleFactor * ballDir.y +
                      (-ballDir.y * BLOCK_BALL_LOCATION_OFFSET);
    blockLocation.x = criticalValue + (-ballDir.x * BLOCK_BALL_LOCATION_OFFSET);

    ballDir.x = ballDir.x * -1;  // reverse the ball velocity to get the
                                 // goalie's blocking orientation
    ballDir.y = ballDir.y * -1;
    blockOri  = ballDir.orientation();

    if (blockLocation.y > world.field().friendly_goalpost_pos().y *
                              BLOCK_BALL_GOAL_EXTENSION_FACTOR ||
        blockLocation.y < world.field().friendly_goalpost_neg().y *
                              BLOCK_BALL_GOAL_EXTENSION_FACTOR)
    {
        // error checking: if the block location is not within the goal posts
        // then stay in the same position/orientation
        // Goalie::rejectBall(ca, world);
        blockLocation = player().position();
        blockOri      = player().orientation();
    }

    if (abs((player().position() - world.ball().position()).len()) <
            BLOCK_REJECT_DISTANCE_THRESHHOLD &&
        world.ball().velocity().len() < REJECT_BALL_VELOCITY_THRESHHOLD &&
        Evaluation::ball_in_friendly_crease(world))
    {
        Goalie::rejectBall(ca, world);
    }
    else
    {
        // if the shot has a large y value, and a small x value it is likely
        // shot from close to the endline. If this is the case, pull the goalie
        // out further
        if (abs(ballDir.y) >= LOWPROFILE_SHOT) //FIXME not a good solution
        {
            if (ballDir.y >= 0)
            {  // set the critical value depending on the direction of the shot
                // (neg/pos goal y-value)
                criticalValue = world.field().friendly_goalpost_pos().y;
            }
            else
            {
                criticalValue = world.field().friendly_goalpost_neg().y;
            }

            // extrapolate the ball direction to find a new location for the
            // goalie.
            scaleFactor = (criticalValue - blockLocation.y) / (-1 * ballDir.y);
            blockLocation.y = criticalValue;
            blockLocation.x = blockLocation.x + -1 * ballDir.x * scaleFactor;
        }

        goalieStatePos = blockLocation;
        goalieStateAngle = blockOri;

        Action::move(
            ca, world, player(), blockLocation,
            blockOri);  // move the goalie to block the ball
    }
}

void GoalieAssistant1::blockShot(World world, caller_t&) {
    Point ballDir = world.ball().velocity().norm();
}

void GoalieAssistant2::blockShot(World world, caller_t&) {
    Point ballDir = world.ball().velocity().norm();
    Point blockDir = ballDir.perp();
    Point ballPos = world.ball().position();

    //ballDir.x*player().position().x + ballDir.y*player().position().y + world.ball().position();
    double yIntercept;
    double scale;

    // 0 = ballPos + ballDir.x*scale
    scale = -ballPos.x /ballDir.x;

    //


}
void Goalie::blockAimedShot(caller_t& ca, World world)
{
    Robot enemyBaller;
    Point shotDir;
    Point blockPos;
    double criticalValue;
    double scaleFactor;
    Angle blockOri;

    enemyBaller = Evaluation::calc_enemy_baller(world);
    if (enemyBaller.position().x < world.field().friendly_crease_pos_corner().x + Robot::MAX_RADIUS) {  // if the ball is below the top of the crease, use special logic

        Point blockPos = Evaluation::evaluateShallowAngleBlock(world, Evaluation::calc_enemy_baller(world).position());
        blockOri = (world.ball().position() - player().position()).orientation();
        goalieStatePos = blockPos;
        goalieStateAngle = blockOri;

        Action::move(ca, world, player(), blockPos, blockOri);
    LOG_INFO("NEW RUNS");
    } else {
        LOG_INFO("OLD RUNS");
        criticalValue = world.field().friendly_goal().x;

        shotDir = (world.ball().position() - enemyBaller.position()).norm();

        scaleFactor = (criticalValue - enemyBaller.position().x) / shotDir.x;

        blockPos.y = enemyBaller.position().y + scaleFactor * shotDir.y +
                     (-shotDir.y * BLOCK_BALL_LOCATION_OFFSET);
        blockPos.x = criticalValue + (-shotDir.x * BLOCK_BALL_LOCATION_OFFSET);

        shotDir.x = shotDir.x * -1;  // reverse the ball velocity to get the
        // goalie's blocking orientation
        shotDir.y = shotDir.y * -1;
        blockOri  = shotDir.orientation();

        if (blockPos.y > world.field().friendly_goalpost_pos().y ||
            blockPos.y < world.field().friendly_goalpost_neg().y ||
            blockPos.x < criticalValue)
        {
            // error checking: if the block location is not within the goal posts
            // if the ball doesn't look like it will go in the net, just shadow ball
            // Goalie::rejectBall(ca, world);
            shadowBall(world, ca);
            LOG_INFO("prep-shadow");
        }
        else
        {
            goalieStatePos = blockPos;
            goalieStateAngle = blockOri;

            Action::move(ca, world, player(), blockPos, blockOri);
            // TODO: add a factor that considers the direction of the robot AND the direction of the vector between the enemy robot and ball
        }

    }

}

// evaluate whether it is possible that an enemy robot is looking to aim a shot
// on net
bool Goalie::enemyPreparingShot(World world)
{
    bool preparingShot = false;


    if (world.enemy_team().size() == 0) // check that there are enemy bots before indexing array before indexing array
    {
        return false;
    }

    else
    {
        Robot closestEnemy = Evaluation::calc_enemy_baller(world);  // grab the closest enemy to the ball

        double distance =
            (world.ball().position() - closestEnemy.position()).len();

        bool facingFriendlyGoal =
                (closestEnemy.orientation().abs() >=
             Angle::of_degrees(PREPARING_SHOT_ORIENTATION_TRESHHOLD)) &&
            (closestEnemy.orientation() <=
             Angle::of_degrees(360-PREPARING_SHOT_ORIENTATION_TRESHHOLD));

        bool movingSlow = closestEnemy.velocity().len() <=
                          PREPARING_SHOT_ROBOTSPEED_THRESHHOLD;
        bool slowBall =
            world.ball().velocity().len() < PREPARING_SHOT_BALLSPEED_THRESHHOLD;

        bool closeToBall = distance <= PREPARING_SHOT_PROXIMITY_THRESHHOLD;

        preparingShot =
            facingFriendlyGoal && movingSlow && slowBall && closeToBall;

        // for debugging purposes only
        //LOGF_INFO("facing:%1, plaSLow:%2, balSlow:%3, close2Bal:%4, ballerAng:%5", facingFriendlyGoal, movingSlow, slowBall, closeToBall, closestEnemy.orientation().to_degrees());

        return preparingShot;
    }
}

double Goalie::goalieDefendPosScale(World world){

}

void Goalie::goalieMove(
    caller_t& ca, World world, Point dest, Angle orientation)
{
    float positionDifference;

    positionDifference = (player().position() - dest).lensq();

    if (positionDifference < JITTER_THRESHHOLD)
    {
        Action::move(
            ca, world, player(), player().position(), player().orientation());
    }
    else
    {
        Action::move(ca, world, player(), dest, orientation);
    }
}

void Goalie::goaliePositionPreparingShot(World world, caller_t& ca)
{
    //    Point ballLocation = world.ball().position();
    //    auto robotList     = world.enemy_team();
    //    Robot closestEnemy =
    //        robotList[0];  // default value for the closest enemy to the ball
    //    double minDistance = dist(ballLocation, closestEnemy.position());
    //    double distance = 999;  // very large magic number to initialize
    //    distance so
    //                            // that any enemy robot is closer to the ball
    //    Point goalieDest = world.field().friendly_goal();  // default position
    //    of
    //                                                       // the goalie is
    //                                                       the
    //                                                       // center of the
    //                                                       net
    //
    //    // find the closest enemy robot to the ball
    //    for (auto robo : world.enemy_team())
    //    {
    //        distance = dist(robo.position(), ballLocation);
    //
    //        if (distance < minDistance)
    //        {
    //            closestEnemy = robo;
    //            minDistance  = distance;
    //        }
    //    }
    //
    //    Point shotDirection =
    //        ballLocation - closestEnemy.position();  // direction = ball -
    //        enemy
    //
    //    goalieDest = goaliePosition(world, shotDirection);  // position goalie
    //                                                        // depending on
    //                                                        where
    //                                                        // the ball
    //                                                        intersects
    //                                                        // our endline
    //
    //    Angle goalieOrientation =
    //        (world.ball().position() - player().position()).orientation();
    //    AI::HL::STP::Action::move(
    //        ca, world, player(), goalieDest, goalieOrientation);
}

Point Goalie::goaliePositionMovingShot(World world)
{  // positions the goalie if the ball is determined to be heading towards our
    // endline
    Point goalieDes = world.field().friendly_goal();  // default position of the
                                                      // goalie is the center of
                                                      // the net

    Point ballVelocity = world.ball().velocity();

    goalieDes = goaliePosition(world, ballVelocity);  // position goalie to
                                                      // block location where
                                                      // the ball will intersect
                                                      // our endline

    return goalieDes;
}

// returns a unit vector point (acts as just a direction)
Point Goalie::pointNormalize(Point point)
{
    double magnitude = sqrt(
        pow(point.x, 2) + pow(point.y, 2));  // magnitude = sqrt( x^2 + y^2 )
    point.x /= magnitude;
    point.y /= magnitude;

    return point;
}

Point Goalie::goaliePosition(World world, Point ballDirection)
{  // calculates where the ball will pass over our goal line and positions the
    // goalie accordingly

    ballDirection = pointNormalize(
        ballDirection);  // get the unit vector of the ball direction
    Point endLineIntersectionPoint = endLineIntersection(world, ballDirection);
    Point goalieDes = world.field().friendly_goal();  // default position of
                                                      // goalie is in the center
                                                      // of the friendly net

    // cases:
    //  greater than top of goal -> go to high end
    //  less than bottom of goal -> go to low end
    //  in the goal -> go to intersection and add set radius in direction of the
    //  ball
    if (endLineIntersectionPoint.y >= world.field().friendly_corner_pos().y)
    {  // if the shot misses the net high, then position robot near the upper
        // corner
        goalieDes.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
        goalieDes.y =
            world.field().friendly_crease_pos_corner().y - Robot::MAX_RADIUS;
    }
    else if (
        endLineIntersectionPoint.y <= world.field().friendly_corner_neg().y)
    {  // if the shot misses the net low, then position robot near the lower
        // corner
        goalieDes.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
        goalieDes.y =
            world.field().friendly_crease_neg_corner().y + Robot::MAX_RADIUS;
    }
    else
    {  // if the shot will land in the net, then position the goalie between the
        // intersection point and the ball
        goalieDes.x =
            endLineIntersectionPoint.x - GOALIE_DEFEND_RADIUS * ballDirection.x;
        goalieDes.y = world.field().friendly_goal().y -
                      GOALIE_DEFEND_RADIUS * ballDirection.y;
    }

    return goalieDes;
}

Point Goalie::endLineIntersection(World world, Point ballDirection)
{
    Point goalTopCorner = world.field().friendly_corner_pos();
    double goalXValue   = goalTopCorner.x;
    Point goalIntersection =
        world.field().friendly_goal();  // default location of the goalie is in
                                        // the center of friendly net

    ballDirection = pointNormalize(
        ballDirection);  // get the unit vector of the ball direction

    // goalX = ballPos + n*ballDirection
    double directionScaler =
        ((goalXValue - world.ball().position().y) / ballDirection.x);

    double goalIntersectionY =
        world.ball().position().y - directionScaler * ballDirection.y;
    goalIntersection.x = goalXValue;
    goalIntersection.y = goalIntersectionY;

    return goalIntersection;
}

Robot Goalie::isEnemyPassing(World world)
{  // calculates and returns the most likely enemy robot to be receiving a pass
    auto enemyList = world.enemy_team();
    // calculate if the ball is heading towards a robot
    // calculate if there is a robot that is heading to a pass
    // check position and direction of enemy robot to help determine a pass

    Point ballVelocity = world.ball().velocity();
    ballVelocity =
        pointNormalize(ballVelocity);  // calculate the direction of the ball

    // need to calculate a cone of points in the ball direction. locate enemy
    // robots in the cone
    //  transform all enemy locations and ball velocity to alpha-beta coordinate
    //  system
    //  calculate alpha positions of enemy robots and check to see if they are
    //  inside the shot-cone by checking the coresponding max beta value for
    //  said alpha
    //  after all robots in the cone are detectted, check orientations and
    //  velocities
    //
    // TODO: need to return to this function after transform function is done
    //  ONLY POSITIVE x' can be passed to
    return enemyList[1];
}

Point Goalie::transformToBallAxis(Point coordinate, Point direction)
{
    Point transfCoordinate;                 // initialize transformed coordinate
    direction = pointNormalize(direction);  // onvert to unit point in case
                                            // function is passed a larger
                                            // magnitude

    double theta = atan2(
        direction.y,
        direction.x);  // angle of the direction from the x-axis in radians

    // x' = xcos(0) + ysin(0) (x' is the direction of the ball)
    // y' = -xsin(0) + ycos(0)
    transfCoordinate.x = coordinate.x * cos(theta) + coordinate.y * sin(theta);
    transfCoordinate.y =
        -1 * coordinate.x * sin(theta) + coordinate.y * cos(theta);

    return transfCoordinate;
}
/*
double Goalie::passConeXtoY(double xPrime, double theta) {      // used to
calculare the maximum y' value from the direction of the ball to be considered
within possible receiving distance
    double yPrime;

    yPrime = tan(theta) * xPrime;      // tan(0) = y'/x'

    return yPrime;
}
*/
std::vector<Robot> Goalie::potentialEnemyPassReceivers(World world)
{  // removes all non-positive x' axis enemy robots SEEME: fix return type here
    // and at declaration

    double
        transfXPrime;  // variable to hold the x' value for one enemy at a time
    double
        transfYPrime;  // variable to hold the y' value for one enemy at a time
    std::vector<Robot> potentialReceivers;  // vector container to hold all of
                                            // the potential enemy receivers
    Point robotPositionTransf;
    Point robotVelocityTransf;

    for (auto robo : world.enemy_team())
    {
        robotPositionTransf =
            transformToBallAxis(robo.position(), world.ball().velocity());
        transfXPrime = robotPositionTransf.x;

        if (transfXPrime > PASSING_DISTANCE_THRESHHOLD)
        {
            // check to see if the robot in question is in the direct path of
            // the ball
            robotVelocityTransf = pointNormalize(robo.velocity());
            transfYPrime        = robotPositionTransf.y;
            if (fabs(transfYPrime) <=
                passConeXToY(transfXPrime, PASSING_CONE_ANGLE))
            {
                potentialReceivers.push_back(robo);
                break;
            }

            // check to see if the enemy is moving towards the path of the ball
            // transform robot velocity to x',y' axis
            else if (
                (robotVelocityTransf.y > 0 && robotPositionTransf.y <= 0) ||
                (robotVelocityTransf.y < 0 && robotPositionTransf.y >= 0))
            {
                potentialReceivers.push_back(robo);
                // this "if" block adds the enemy robot as a potential receiver
                // if it is moving towards the perpendicular axis to the motion
                // of the ball
                // enemies in +y' must have a -y' velocity to move towards the
                // ball, and vice-versa for -y' position
            }
        }
    }

    potentialReceivers =
        distanceToBallSort(world.ball().position(), potentialReceivers);

    return potentialReceivers;
}

std::vector<Robot> Goalie::distanceToBallSort(
    Point ballPosition, std::vector<Robot> roboList)
{  // TODO: make this more efficient in the future. There are ways to sort
    // without recalculating distance
    double distance;
    double currentMinDistance;
    unsigned int j = 0;     // iterator for length of the intital roboList
    unsigned int i = 0;     // iterator for the length of the modified roboList
    unsigned int minIndex;  // holds the index of the minDistance robot to be
                            // erased from the list later

    std::vector<Robot> sortedRoboList;
    for (i = 0; i < roboList.size(); i++)
    {  // loop# is equivalent to the number of elements in the list

        currentMinDistance =
            dist(roboList[0].position(), ballPosition);  // set the default
                                                         // minimum distance to
                                                         // the first robot

        for (j = 0; j < roboList.size(); j++)
        {  // in each loop iteration look for the smallest distance
            distance = dist(roboList[j].position(), ballPosition);

            if (currentMinDistance >= distance)
            {
                currentMinDistance = distance;
                minIndex           = j;
            }  // save the index of the new minimum distance
        }

        sortedRoboList.push_back(roboList[int(j)]);  // put the closest robot
                                                     // (in corresponding
                                                     // iteration) to the back
                                                     // of the list
        roboList.erase(roboList.begin() + int(minIndex));  // delete the robot
                                                           // that has just been
                                                           // placed into the
                                                           // sorted list so
                                                           // that it doesn't
                                                           // duplicate
    }

    return sortedRoboList;
}

}  // anonymous namespace closing brace

Tactic::Ptr AI::HL::STP::Tactic::goalie(W::World world)
{
    return Tactic::Ptr(new Goalie(world));
}

Tactic::Ptr AI::HL::STP::Tactic::goalieAssist1(W::World world)
{
    return Tactic::Ptr(new GoalieAssistant1(world));
}

Tactic::Ptr AI::HL::STP::Tactic::goalieAssist2(W::World world)
{
    return Tactic::Ptr(new GoalieAssistant2(world));
}

Tactic::Ptr AI::HL::STP::Tactic::ballerBlocker(W::World world) {

    return Tactic::Ptr(new BallerBlocker(world));
}