#include "ai/hl/stp/action/stop.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::stop(caller_t& ca, World, Player player)
{
    player.prio(AI::Flags::MovePrio::LOW);

    const Primitive& prim =
        Primitives::Move(player, player.position(), player.orientation());
    Action::wait(ca, prim);
}
