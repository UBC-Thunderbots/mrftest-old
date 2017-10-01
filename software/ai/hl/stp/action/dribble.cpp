#include "dribble.h"

#include "ai/flags.h"
#include "ai/hl/stp/action/action.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace
{
DoubleParam angle_increment(
    u8"dribble angle increment (deg)", u8"AI/HL/STP/Action/Dribble", 10, 1,
    100);
DoubleParam linear_increment(
    u8"dribble linear increment", u8"AI/HL/STP/Action/Dribble", 0.05, 0.001, 1);
}

#warning these should not use the old HL primitives
void AI::HL::STP::Action::dribble(
    caller_t& ca, World world, Player player, Point dest)
{
    AI::HL::STP::Action::dribble(
        ca, world, player, dest,
        (world.ball().position() - player.position()).orientation());
}

void AI::HL::STP::Action::dribble(
    caller_t& ca, World world, Player player, Point dest, Angle ori)
{
    Primitives::Primitive::Ptr(
        new Primitives::Dribble(player, dest, ori, false));
}

void AI::HL::STP::Action::dribble(
    caller_t& ca, World world, Player player, Point dest, Point face)
{
    AI::HL::STP::Action::dribble(
        ca, world, player, dest, (face - player.position()).orientation());
}

void AI::HL::STP::Action::dribble(caller_t& ca, Player player)
{
    Primitives::Primitive::Ptr(new Primitives::Dribble(
        player, player.position(), player.orientation(), false));
}
