#include "ai/hl/stp/action/move.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/action.h"

using namespace AI::HL::STP;

// if should_wait is false, robot stops after reaching destination
void AI::HL::STP::Action::move(
    caller_t& ca, World world, Player player, Point dest, bool should_wait)
{
    Primitive prim = Primitives::Move(
        player, dest, (dest - player.position()).orientation());

    // hold the primitive and yield until it is done if should_wait
    if (should_wait)
    {
        while (!prim.done())
            yield(ca);
    }

    yield(ca);
}

void AI::HL::STP::Action::move(
    caller_t& ca, World, Player player, Point dest, Angle orientation,
    bool should_wait)
{
    Primitive prim = Primitives::Move(player, dest, orientation);

    // if should_wait, hold the primitive and yield until it is done
    if (should_wait)
    {
        while (!prim.done())
            yield(ca);
    }

    yield(ca);
}

void AI::HL::STP::Action::move_dribble(
    caller_t& ca, World world, Player player, Angle orientation, Point dest,
    bool should_wait)
{
    Primitive prim = Primitives::Dribble(player, dest, orientation, false);

    // if should_wait, hold the primitive and yield until it is done
    if (should_wait)
    {
        while (!prim.done())
            yield(ca);
    }

    yield(ca);
}

void AI::HL::STP::Action::move_careful(
    caller_t& ca, World world, Player player, Point dest, bool should_wait)
{  // move "carefully" (slowly)
    player.flags(player.flags() | AI::Flags::MoveFlags::CAREFUL);

    Primitive prim = Primitives::Move(
        player, dest,
        (world.ball().position() - player.position()).orientation());

    // if should_wait, hold the primitive and yield until it is done
    if (should_wait)
    {
        while (!prim.done())
            yield(ca);
    }

    yield(ca);
}
