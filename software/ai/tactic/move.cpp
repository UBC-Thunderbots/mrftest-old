#include "ai/tactic/move.h"

move::move(player::ptr player, world::ptr world) : tactic(player), navi(player, world), position_initialized(false), orientation_initialized(false) {
}

move::move(player::ptr player, world::ptr world, const unsigned int& flags) : tactic(player, flags), navi(player, world), position_initialized(false), orientation_initialized(false) {
}

//move::move(player::ptr player, world::ptr world, const unsigned int& flags, const point& position) : tactic(player, flags), navi(player, world), target_position(position), position_initialized(true), orientation_initialized(false) {
//}

void move::tick() {
	if (position_initialized) navi.set_position(target_position);
	if (orientation_initialized) navi.set_orientation(target_orientation);
	navi.set_flags(flags);
	navi.tick();
}

