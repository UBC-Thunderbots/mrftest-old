#include <algorithm>

#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/enemy.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/util.h"

namespace Primitives = AI::BE::Primitives;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
using AI::HL::STP::Coordinate;
using namespace AI::HL::Util;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action     = AI::HL::STP::Action;

namespace
{
class ShootGoal final : public Tactic
{
   public:
    explicit ShootGoal(World world) : Tactic(world)
    {  // constructor
    }

   private:
    void execute(caller_t& ca) override;
    Player select(const std::set<Player>& players) const override;
    void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const override;

    Glib::ustring description() const override
    {
        return u8"shoot-goal";
    }
};

class ShootTarget final : public Tactic
{
   public:  // shoots at target with specific coordinate
    explicit ShootTarget(World world, const Coordinate target)
        : Tactic(world), target(target)
    {
    }

   private:
    Coordinate target;

    void execute(caller_t& ca) override;
    Player select(const std::set<Player>& players) const override;

    Glib::ustring description() const override
    {
        return u8"shoot-target";
    }
};

void ShootGoal::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const
{
    ctx->set_source_rgba(1.0, 0.7, 0.7, 0.8);
    ctx->move_to(player().position().x, player().position().y);
    Point f = Evaluation::get_best_shot(world, player());
    ctx->line_to(f.x, f.y);
    ctx->stroke();
}

Player ShootGoal::select(const std::set<Player>& players) const
{
    return select_baller(world, players, player());
}

void ShootGoal::execute(caller_t& ca)
{
    while (true)
    {
        std::vector<Point> obstacles;
        Point target;

        for (auto i : world.enemy_team())
        {
            obstacles.push_back(i.position());
        }
        for (auto i : world.friendly_team())
        {
            if (i != player())
            {
                obstacles.push_back(i.position());
            }
        }

        bool chip =
            Evaluation::get_best_shot_pair(world, player()).second.abs() <
                Angle::of_degrees(5) &&
            player().position().x > 0;
        // TODO; If chipping will want to adjust power instead of always passing
        // ball max speed (that would be an 8m chip if possible)
        chip = false;  // THIS IS TEMPORARY FOR TESTING

        Evaluation::ShootData shoot_data =
            Evaluation::evaluate_shoot(world, player(), true);
        AI::HL::STP::Action::catch_and_shoot_target(
            ca, world, player(), shoot_data.target, AI::HL::STP::BALL_MAX_SPEED,
            chip);
        yield(ca);
    }
}

Player ShootTarget::select(const std::set<Player>& players) const
{
    return select_baller(world, players, player());
}

void ShootTarget::execute(caller_t& ca)
{
    while (true)
    {
        AI::HL::STP::Action::catch_and_shoot_target(
            ca, world, player(), target.position());
        yield(ca);
    }
}
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_goal(World world)
{
    Tactic::Ptr p(new ShootGoal(world));
    return p;
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_target(
    World world, const Coordinate target)
{
    Tactic::Ptr p(new ShootTarget(world, target));
    return p;
}
