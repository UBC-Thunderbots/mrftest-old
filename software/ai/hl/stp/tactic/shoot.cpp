#include "ai/hl/stp/action/shoot.h"
#include <algorithm>
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/tactic/shoot.h"

using namespace AI::HL::STP::Tactic;
namespace W          = AI::HL::W;
using Coordinate     = AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action     = AI::HL::STP::Action;

#define SHOOTING_DEGREE_TOL 5

namespace
{  // anonymous namespace for encapsulation

class ShootGoal final : public Tactic
{   // class for having the robot shoot at the opposing teams net (Shooting a
    // goal)
   public:
    explicit ShootGoal(W::World world) : Tactic(world)
    {
    }  // constructor

   private:
    void execute(caller_t& ca) override;  // Mandatory
    W::Player select(
        const std::set<W::Player>& players) const override;  // virtual
    Glib::ustring description() const override
    {
        return u8"Shoot Goal";
    }  // functions
    void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx)
        const;  // overlay for desired shot path
};

class ShootTarget final : public Tactic
{  // class for having the robot shoot at a target location (Point type)
   public:
    explicit ShootTarget(
        W::World world, const Coordinate target, double power, bool bChip)
        : Tactic(world), target(target), power(power), bChip(bChip)
    {
    }  // constructor

   private:
    Coordinate target;  // target to be shot at
    double power;
    bool bChip;

    void execute(caller_t& ca) override;  // Mandatory
    W::Player select(
        const std::set<W::Player>& players) const override;  // virtual
    Glib::ustring description() const override
    {
        return u8"Shoot Target";
    }  // functions
};

void ShootGoal::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const
{  // function draws a line from the player to the calculated best shot on net
    ctx->set_source_rgba(1.0, 0.7, 0.7, 0.8);
    ctx->move_to(player().position().x, player().position().y);
    Point f = Evaluation::get_best_shot(world, player());
    ctx->line_to(f.x, f.y);
    ctx->stroke();
}

W::Player ShootGoal::select(const std::set<W::Player>& players) const
{
    return select_baller(world, players, player());
}

void ShootGoal::execute(caller_t& ca)
{
    while (true)
    {
        bool bEvaluateAngle =
            Evaluation::get_best_shot_pair(world, player()).second.abs() <
            Angle::of_degrees(SHOOTING_DEGREE_TOL);  // check to see if he
                                                     // shooting angle is less
                                                     // than 5 degrees
        bool bPlayerOnEnemySide = player().position().x > 0;

        bool bChip = bEvaluateAngle && !bPlayerOnEnemySide;

        double shotPower;

        Evaluation::ShootData shootData = Evaluation::evaluate_shoot(
            world, player(), true);  // evaluate_shoot tries to select the best
                                     // shot on net the boolean parameter is for
                                     // the reduced or large shoot radius

        if (bChip)
        {
            shotPower =
                Evaluation::calc_chip_power(world, player(), shootData.target);
        }
        else
        {
            shotPower = AI::HL::STP::BALL_MAX_SPEED;
        }

        Action::catch_and_shoot_target(
            ca, world, player(), shootData.target, shotPower,
            bChip);  // catch_and_shoot actually performs the act of getting
                     // behind the ball and shooting
        yield(ca);
    }
}

W::Player ShootTarget::select(const std::set<W::Player>& players) const
{
    return select_baller(world, players, player());
}

void ShootTarget::execute(caller_t& ca)
{
    while (true)
    {
        Action::catch_and_shoot_target(
            ca, world, player(), target.position(), power, bChip);

        yield(ca);
    }
}
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_goal(W::World world)
{
    Tactic::Ptr p(new ShootGoal(world));
    return p;
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_target(
    W::World world, const Coordinate target, double power, bool bChip)
{
    Tactic::Ptr p(new ShootTarget(world, target, power, bChip));
    return p;
}
