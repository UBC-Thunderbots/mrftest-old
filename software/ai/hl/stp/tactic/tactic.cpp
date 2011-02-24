#include "ai/hl/stp/tactic/tactic.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Tactic::Tactic(const World &world, bool active) : world(world), active_(active) {
}

Tactic::~Tactic() {
}

bool Tactic::done() const {
	assert(0);
}

Player::Ptr Tactic::select(const std::set<Player::Ptr> &players) {
	assert(0);
}

void Tactic::set_player(Player::Ptr p) {
	if (player != p) {
		player = p;
		player_changed();
	}
}

std::string Tactic::description() const {
	return "";
}

void Tactic::player_changed() {
}

