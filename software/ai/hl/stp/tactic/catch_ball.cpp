#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/tactic/catch_ball.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/action/catch.h"

namespace Primitives = AI::BE::Primitives;
namespace Evaluation = AI::HL::STP::Evaluation;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace
{
class CatchBall final : public Tactic::Tactic
{
   public:
    explicit CatchBall(World world) : Tactic(world)
    {
    }

   private:
    Player select(const std::set<Player>& players) const override;

    void execute(caller_t& caller) override;

    Glib::ustring description() const override
    {
        return u8"catch ball";
    }
};

Player CatchBall::select(const std::set<Player>& players) const
{
    return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpScalar<Player, double>(
            [this](Player p){ return (p.position() - Evaluation::baller_catch_position(world, p)).len(); }
    ));

}

// executes caller on selected player

void CatchBall::execute(caller_t& caller)
{
    do {
        AI::HL::STP::Action::catch_ball(caller, world, player());
        yield(caller);
    } while(true);
}
}
// creates idle world for selected player by calling superclass
Tactic::Ptr AI::HL::STP::Tactic::catch_ball(World world)
{
    return Tactic::Ptr(new CatchBall(world));
}
