#include "ai/tactic/move.h"

Move::Move(Player::Ptr player, World::Ptr world) : Tactic(player), navi(player, world), position_initialized(false), orientation_initialized(false) {
}

Move::Move(Player::Ptr player, World::Ptr world, const unsigned int& flags) : Tactic(player, flags), navi(player, world), position_initialized(false), orientation_initialized(false) {
}

//Move::Move(Player::Ptr player, World::Ptr world, const unsigned int& flags, const Point& position) : Tactic(player, flags), navi(player, world), target_position(position), position_initialized(true), orientation_initialized(false) {
//}

void Move::tick() {
	if (position_initialized) navi.set_position(target_position);
	if (orientation_initialized) navi.set_orientation(target_orientation);
	navi.set_flags(flags);
	navi.tick();
}

