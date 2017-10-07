#include <algorithm>

#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/indirect_chip.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/tactic/prepare_kick.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"
using namespace std;

namespace Primitives = AI::BE::Primitives;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::Util;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action     = AI::HL::STP::Action;
using namespace Geom;

namespace
{
DoubleParam CHIP_POWER_EXP(
    u8"test controls exponential change in chipping power as a function of "
    u8"distance",
    u8"AI/HL/STP/Tactic/prepare_kick", 1.0, 0.0, 100.0);
DoubleParam CHIP_POWER_SCALING(
    u8"test controls scaling factor of chip power as function of distance",
    u8"AI/HL/STP/Tactic/prepare_kick", 0.4, 0.0, 100.0);

class PrepareKick final : public Tactic
{
   public:
    explicit PrepareKick(World world) : Tactic(world)
    {
    }

   private:
    void execute(caller_t& ca) override;
    Player select(const std::set<Player>& players) const override;

    Glib::ustring description() const override
    {
        return u8"prepare-kick";
    }
};

Player PrepareKick::select(const std::set<Player>& players) const
{
    return select_baller(world, players, player());
}

void PrepareKick::execute(caller_t& ca)
{
    while (true)
    {
        // Action::catch_stopped_ball(ca, world, player());

        Point gp1         = world.field().enemy_goal_boundary().first;
        Point gp2         = world.field().enemy_goal_boundary().second;
        Angle total_angle = vertex_angle(gp1, world.ball().position(), gp2);
        total_angle       = total_angle.angle_mod().abs();
        Angle shot_angle =
            Evaluation::get_best_shot_pair(world, player()).second;
        shot_angle = shot_angle.abs();

        if (shot_angle / total_angle > 0.2)
        {  // robot has space to shoot
            Action::shoot_target(
                ca, world, player(),
                Evaluation::get_best_shot(world, player()));
        }
        else
        {  // robot will try chip instead, should be given new target
            Point target;
            double chip_power;
            if (player().position().x < 0.0)
            {  // in our end. Will aim for centre of enemy net
                target = world.field().enemy_goal();
            }
            else
            {  // in enemy's end. Will aim for best shot at enemy net. Should
                // not consider first blocking player
                target =
                    Evaluation::indirect_chip_target(world, player()).first;
            }

            // second formula is a temporary modifier, since chipping is not
            // currently accurately calibrated
            chip_power =
                (target - world.ball().position()).len() *
                (CHIP_POWER_SCALING *
                 pow((world.field().enemy_goal() - world.ball().position())
                         .len(),
                     CHIP_POWER_EXP));

            LOGF_INFO(u8"TARGET: %1, POWER: %2", target, chip_power);

            // overrideing targeting for now, due to chipping/evaluation not
            // being accurate
            Action::shoot_target(
                ca, world, player(), world.field().enemy_goal(), chip_power,
                true);
        }
        yield(ca);
    }
}
}

Tactic::Ptr AI::HL::STP::Tactic::prepare_kick(World world)
{
    Tactic::Ptr p(new PrepareKick(world));
    return p;
}
