#include "ai/hl/stp/action/intercept.h"
#include "ai/flags.h"
#include "ai/hl/util.h"

BoolParam INTERCEPT_MP_MOVE(
    u8"Use mp_move or MP_intercept", u8"AI/HL/STP/Action/Shoot", true);

void AI::HL::STP::Action::intercept(
    caller_t& ca, World, Player player, const Point target)
{
    // mp_catch(world.ball().position());
    //
    if (INTERCEPT_MP_MOVE)
    {
        AI::BE::Primitives::Ptr prim(new Primitives::Move(
            player, target, (target - player.position()).orientation()));
        (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
        // Action::wait(ca, prim);
        // while ((player.position() - target).len() > AI::HL::Util::POS_CLOSE)
        //     yield(ca);
        // player.clear_prims();
    }
    else
    {
        AI::BE::Primitives::Ptr prim(new Primitives::Dribble(
            player, target, (target - player.position()).orientation(), false));
        (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);

        // Action::wait(ca, prim);
        // while ((player.position() - target).len() > AI::HL::Util::POS_CLOSE)
        //     yield(ca);
        // player.clear_prims();
    }
}