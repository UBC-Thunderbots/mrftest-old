#include "ai/hl/stp/action/move.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

// if should_wait is false, robot stops after reaching destination (Not
// currently implemented)

void AI::HL::STP::Action::move(
    caller_t& ca, World world, Player player, Point dest, bool should_wait)
{  // no orientation
    Primitive prim = Primitives::Move(
        player, dest, (dest - player.position()).orientation());
    yield(ca);
}

void AI::HL::STP::Action::move(
    caller_t& ca, World, Player player, Point dest, Angle orientation,
    bool should_wait)
{  // with orientation
    Primitive prim = Primitives::Move(player, dest, orientation);
    yield(ca);
}

void AI::HL::STP::Action::move_dribble(
    caller_t& ca, World world, Player player, Angle orientation, Point dest,
    bool should_wait)
{  // with dribbler ON and orientation
    Primitive prim = Primitives::Dribble(player, dest, orientation, false);
    yield(ca);
}

void AI::HL::STP::Action::move_careful(
    caller_t& ca, World world, Player player, Point dest, bool should_wait)
{  // move "carefully" (slowly)
    player.flags(player.flags() | AI::Flags::MoveFlags::CAREFUL);

    Primitive prim = Primitives::Move(
        player, dest,
        (world.ball().position() - player.position()).orientation());
    yield(ca);
}
