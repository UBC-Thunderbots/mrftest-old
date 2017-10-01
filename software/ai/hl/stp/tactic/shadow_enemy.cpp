#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::Util;
using AI::HL::STP::Coordinate;
using AI::HL::STP::Enemy;
namespace Action = AI::HL::STP::Action;

namespace
{
class ShadowEnemy final : public Tactic
{
   public:
    explicit ShadowEnemy(World world, unsigned int index)
        : Tactic(world), index(index)
    {
    }

   private:
    Coordinate dest;
    Player select(const std::set<Player> &players) const override;
    unsigned int index;
    void execute(caller_t &ca) override;
    Glib::ustring description() const override
    {
        return u8"shadow_enemy";
    }
};

Player ShadowEnemy::select(const std::set<Player> &players) const
{
    return *std::min_element(
        players.begin(), players.end(),
        AI::HL::Util::CmpDist<Player>(dest.position()));
}

void ShadowEnemy::execute(caller_t &ca)
{
    while (true)
    {
        std::vector<AI::HL::STP::Evaluation::Threat> enemy_threats =
            AI::HL::STP::Evaluation::calc_enemy_threat(world);
        if (enemy_threats[index].robot)
        {
            Point enemy_to_block = enemy_threats[index].robot.position();

            /**
             * ssshh... global state
             * DO NOT TOUCH THIS unless you know what you are doing.
             */
            bool goalie_top = true;

            const Field &field = world.field();
            const Point goal_side =
                goalie_top
                    ? Point(-field.length() / 2, field.goal_width() / 2)
                    : Point(-field.length() / 2, -field.goal_width() / 2);
            const Point goal_opp =
                goalie_top ? Point(-field.length() / 2, -field.goal_width() / 2)
                           : Point(-field.length() / 2, field.goal_width() / 2);

            Point destination = calc_block_cone(
                goal_side, goal_opp, enemy_to_block, Robot::MAX_RADIUS);
            /*		Point ball = world.ball().position();
                            Point destination = ball - enemy;
                            Point rectangle[4];
                            Point line_segment[2];

                            line_segment[0] = enemy;
                            line_segment[1] = world.field().friendly_goal();

                            rectangle[0] = Point(world.field().friendly_goal().x
               + world.field().goal_width() / 2,
               world.field().friendly_goal().y);
                            rectangle[1] = Point(world.field().friendly_goal().x
               - world.field().goal_width() / 2,
               world.field().friendly_goal().y);
                            rectangle[2] = enemy;
                            rectangle[3] = enemy;

                            destination = destination.norm() *
               (AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS);
                            destination = ball - destination;
                            dest = destination;
            */
            Action::move(ca, world, player(), destination);
        }
        yield(ca);
    }
}
}

Tactic::Ptr AI::HL::STP::Tactic::shadow_enemy(World world, unsigned int index)
{
    Tactic::Ptr p(new ShadowEnemy(world, index));
    return p;
}
