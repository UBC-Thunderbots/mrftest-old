#include "ai/hl/stp/action/stop.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::stop(caller_t& ca, World, Player player)
{
    player.prio(AI::Flags::MovePrio::LOW);

    AI::BE::Primitives::Ptr prim(
        new Primitives::Move(player, player.position(), player.orientation()));
    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
}
