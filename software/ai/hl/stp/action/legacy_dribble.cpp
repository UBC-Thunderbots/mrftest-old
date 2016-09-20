#include "ai/hl/stp/action/legacy_action.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/legacy_dribble.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam angle_increment(u8"dribble angle increment (deg)", u8"AI/HL/STP/Action/Dribble", 10, 1, 100);
	DoubleParam linear_increment(u8"dribble linear increment", u8"AI/HL/STP/Action/Dribble", 0.05, 0.001, 1);
}

void AI::HL::STP::Action::dribble(World world, Player player, Point dest) {
	AI::HL::STP::Action::dribble(world, player, dest, (world.ball().position() - player.position()).orientation());
}

void AI::HL::STP::Action::dribble(World world, Player player, Point dest, Angle ori) {
	player.mp_dribble(dest, ori);
}

void AI::HL::STP::Action::dribble(World world, Player player, Point dest, Point face) {
	AI::HL::STP::Action::dribble(world, player, dest, (face - player.position()).orientation());
}

void AI::HL::STP::Action::dribble(Player player) {
	player.mp_dribble(player.position(), player.orientation());
}
