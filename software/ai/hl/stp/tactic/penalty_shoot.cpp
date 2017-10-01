#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace
{
class PenaltyShoot final : public Tactic
{
   public:
    explicit PenaltyShoot(World world)
        : Tactic(world), shoot_up(true), has_shot(false)
    {
    }

   private:
    bool shoot_up;
    bool has_shot;
    bool done() const override;
    Player select(const std::set<Player> &players) const override;
    void execute(caller_t &ca) override;
    Glib::ustring description() const override
    {
        return u8"penalty-shoot";
    }
};

bool PenaltyShoot::done() const
{
    return has_shot;
}

Player PenaltyShoot::select(const std::set<Player> &players) const
{
    return *std::min_element(
        players.begin(), players.end(),
        AI::HL::Util::CmpDist<Player>(world.field().enemy_goal()));
}

void PenaltyShoot::execute(caller_t &ca)
{
    while (true)
    {
        const Field &f = world.field();

        Point enemy_goal       = f.enemy_goal();
        Point enemy_goal_post1 = f.enemy_goal_boundary().first;
        Point enemy_goal_post2 = f.enemy_goal_boundary().second;
        Point target;

        if (world.enemy_team().size() > 0)
        {
            // The enemy team has a goalie - find out which robot is the goalie!
            // Assume that the goalie is the robot closest to enemy goal post
            std::vector<Robot> enemies =
                AI::HL::Util::get_robots(world.enemy_team());

            Robot enemy_goalie = *std::min_element(
                enemies.begin(), enemies.end(),
                AI::HL::Util::CmpDist<Robot>(enemy_goal));

            // Check to see if the enemy goalie is off to one side
            double goal_line_length =
                (enemy_goal_post1 - enemy_goal_post2).len();
            // If goalie is off of the center by threshold don't try fake out
            const double goalie_offset_threshold = goal_line_length * 1 / 3;

            if ((enemy_goalie.position() - enemy_goal).len() >
                goalie_offset_threshold - Robot::MAX_RADIUS)
            {
                // Enemy goalie is further away from the goal center than the
                // threshold. Therefore we should kick in the opposite direction
                if (enemy_goalie.position().y > enemy_goal.y)
                {
                    // Goalie is above the center of the goal. Shoot towards the
                    // bottom of the net.
                    target = Point(f.length() / 2, -goal_line_length / 4);
                }
                else
                {
                    // Goalie is below the center of the goal. Shoot towards the
                    // top of the net.
                    target = Point(f.length() / 2, goal_line_length / 4);
                }
            }
            else
            {
                // Enemy goalie is in the center of the net. Try and fake them
                // out by pivoting.
                AI::HL::STP::Action::move(
                    ca, world, player(),
                    world.ball().position() -
                        Point(Robot::MAX_RADIUS, Robot::MAX_RADIUS),
                    (enemy_goal - world.ball().position() -
                     Point(Robot::MAX_RADIUS, Robot::MAX_RADIUS))
                        .orientation());

                AI::HL::STP::Action::pivot(
                    ca, world, player(), world.ball().position(),
                    Angle::of_radians(0.05));
                target = enemy_goal;
            }
        }
        else
        {
            // Enemy team has no goalie. Shoot straight for the goal!
            target = enemy_goal;
        }

        // This used to be a call to a "Pivot Shoot" action, which seems to no
        // longer exist?
        // Pivot shoot would be useful to implement
        AI::HL::STP::Action::shoot_target(ca, world, player(), target);

        // unset any flags
        player().flags(AI::Flags::MoveFlags::NONE);
        yield(ca);
    }
}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_shoot(World world)
{
    Tactic::Ptr p(new PenaltyShoot(world));
    return p;
}
