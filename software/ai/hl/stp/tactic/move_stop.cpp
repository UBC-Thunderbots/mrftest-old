#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/stop.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "ai/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;

namespace
{
StopLocations stop_locations;
const double AVOIDANCE_DIST =
    AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS + 0.05;

// in ball avoidance, angle between center of 2 robots, as seen from the ball
const Angle AVOIDANCE_ANGLE =
    2.0 * Angle::of_radians(std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST));

const unsigned int NUM_PLAYERS = AI::HL::STP::TEAM_MAX_SIZE - 1;

class MoveStop final : public Tactic
{
   public:
    explicit MoveStop(World world, std::size_t set_player_index)
        : Tactic(world), player_index(set_player_index)
    {
    }

   private:
    Player select(const std::set<Player>& players) const override;

    void execute(caller_t& caller) override;

    std::size_t player_index;

    Glib::ustring description() const override
    {
        return u8"move_stop";
    }
};

Player MoveStop::select(const std::set<Player>& players) const
{
    std::vector<Point> positions = stop_locations(world);
    return *std::min_element(
        players.begin(), players.end(),
        AI::HL::Util::CmpDist<Player>(positions[player_index]));
}

void MoveStop::execute(caller_t& caller)
{
    while (true)
    {
        player().flags(
            player().flags() | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
        std::vector<Point> positions = stop_locations(world);
        // Dribble was changed to act as move stop for robocup 2018. Unhack this
        Action::dribble(caller, world, player(), positions[player_index], (world.ball().position() - player().position()).orientation());
        yield(caller);
    }
}
}

/**
 *
 * \param[in] world the game world
 * \param[in] player_index a zero-index representing one of the players (not
 * including the goalie)
 * \return a pointer to the MoveStop tactic
 */
Tactic::Ptr AI::HL::STP::Tactic::move_stop(
    World world, std::size_t player_index)
{
    Tactic::Ptr p(new MoveStop(world, player_index));
    return p;
}

std::vector<Point> StopLocations::compute(World world)
{
    // draw a circle of radius 50cm from the ball

    // store the ball position and our goal position
    Point ball_pos = world.ball().position();
    Point goal_pos = world.field().friendly_goal();

    Point start;

    // use to store something
    std::vector<Point> positions;

    // check if the ball is in our defense area
    bool in_defense =
        AI::HL::Util::point_in_friendly_defense(world.field(), ball_pos);

    // constrain the average of the ball and goal points to be within the field
    Point def = AI::HL::Util::crop_point_to_field(
        world.field(), (ball_pos + goal_pos) * 0.5);

    if (in_defense)
    {
        // if the ball is in our defense area, then ????????!!!???!???!!!
        start = Point(ball_pos.x + AVOIDANCE_DIST, ball_pos.y);
    }
    else
    {
        // otherwise
        // draw a ray from the center of friendly goal to the ball,
        // and the intersection shall be the start point.

        // unit vector pointing from ball to friendly goal
        Point unitVecBallToGoal = (goal_pos - ball_pos).norm();
        // set the start point to basically be around the ball i guess
        start = ball_pos + unitVecBallToGoal * AVOIDANCE_DIST;

        // TODO: Figure out what these do
        Point def1 = def;
        def1.y += Robot::MAX_RADIUS * 1.5;
        Point def2 = def;
        def2.y -= Robot::MAX_RADIUS * 1.5;

        // ball is in our corner of the field, so we should shift def1 and def2
        // based on that to move them away from
        // our goal
        if (ball_pos.x <= -world.field().length() / 4 &&
            std::fabs(ball_pos.y) > world.field().goal_width() / 2 +
                                        world.field().defense_area_width() / 2)
        {
            if (ball_pos.y < 0)
            {
                def1.y -= Robot::MAX_RADIUS * 3;
                def1.x += Robot::MAX_RADIUS * 3;
            }
            else
            {
                def2.y += Robot::MAX_RADIUS * 3;
                def2.x += Robot::MAX_RADIUS * 3;
            }
        }

        positions.push_back(def1);
        positions.push_back(def2);
    }

    // calculate angle between robots
    const Angle delta_angle = AVOIDANCE_ANGLE + separation_angle;

    const Point shoot = (start - ball_pos);

    // the parity determines left or right
    // we only want one of angle = 0, so start at w = 1
    int w = 1;
    for (std::size_t i = 0; i < NUM_PLAYERS; ++i)
    {
        Angle angle = delta_angle * (w / 2) * ((w % 2) ? 1 : -1);
        Point p     = ball_pos + shoot.rotate(angle);
        ++w;
        positions.push_back(
            AI::HL::Util::crop_point_to_field(world.field(), p));
    }
    return positions;
}
