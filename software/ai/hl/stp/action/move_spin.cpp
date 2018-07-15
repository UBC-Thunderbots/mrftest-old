#include "ai/hl/stp/action/move_spin.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::move_spin(
    caller_t& ca, Player player, Point dest, Angle speed)
{
    AI::BE::Primitives::Ptr prim(new Primitives::Spin(player, dest, speed));

    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
}
