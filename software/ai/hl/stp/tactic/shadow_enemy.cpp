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
    Player select(const std::set<Player> &players) const override;
    unsigned int index;
    void execute(caller_t &ca) override;
    Glib::ustring description() const override
    {
        return u8"shadow_enemy";
    }
};

Point getShadowPosition(World world, Player player, unsigned int index) {
    std::vector<AI::HL::STP::Evaluation::Threat> enemy_threats =
            AI::HL::STP::Evaluation::calc_enemy_threat(world);

    if (enemy_threats[index].robot)
    {
        Point enemy_to_block = enemy_threats[index].robot.position();

        Point destination = enemy_to_block + (enemy_to_block - world.field().friendly_goal()).norm(Robot::MAX_RADIUS * 2);
//        Angle orientation = (enemy_to_block - player.position()).orientation();

        return destination;
    }
}

Player ShadowEnemy::select(const std::set<Player> &players) const
{
    Point dest = getShadowPosition(world, player(), index);
    return *std::min_element(
        players.begin(), players.end(),
        AI::HL::Util::CmpDist<Player>(dest));
}

void ShadowEnemy::execute(caller_t &ca)
{
    while (true)
    {
        Point dest = getShadowPosition(world, player(), index);

//
//        Action::move(ca, world, player(), dest, ori, false, false);

        yield(ca);
    }
}
}

Tactic::Ptr AI::HL::STP::Tactic::shadow_enemy(World world, unsigned int index)
{
    Tactic::Ptr p(new ShadowEnemy(world, index));
    return p;
}
