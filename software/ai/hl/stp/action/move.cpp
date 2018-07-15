#include "ai/hl/stp/action/move.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/action.h"

using namespace AI::HL::STP;

// if should_wait is false, robot stops after reaching destination
void AI::HL::STP::Action::move(
    caller_t &ca, World world, Player player, Point dest)
{
    AI::BE::Primitives::Ptr prim(new Primitives::Move(
        player, dest, (dest - player.position()).orientation()));

    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
}

void AI::HL::STP::Action::move(
    caller_t &ca, World, Player player, Point dest, Angle orientation, bool autokick, bool dribble)
{
    //TODO: delete hack
    //autokick = true;
    uint8_t extra = autokick ? 1 : 0; //value from 0-7
	extra |= (dribble ? 1 : 0) << 1;

    AI::BE::Primitives::Ptr prim(
        new Primitives::Move(player, dest, orientation, extra));

    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
}

void AI::HL::STP::Action::move_overrider(
    caller_t &ca, World, Player player, Point dest, Angle orientation)
{

    AI::BE::Primitives::Ptr prim(new Primitives::Move(player, dest, orientation));
    prim->overrideNavigator = true;
    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
}

void AI::HL::STP::Action::move_dribble(
    caller_t &ca, World world, Player player, Angle orientation, Point dest)
{
    AI::BE::Primitives::Ptr prim(
        new Primitives::Dribble(player, dest, orientation, false));

    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
}

void AI::HL::STP::Action::move_careful(
    caller_t &ca, World world, Player player, Point dest)
{  // move "carefully" (slowly)
    player.flags(player.flags() | AI::Flags::MoveFlags::CAREFUL);

    AI::BE::Primitives::Ptr prim(new Primitives::Move(
        player, dest,
        (world.ball().position() - player.position()).orientation()));

    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
}
