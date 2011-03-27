#include "ai/hl/stp/tactic/tactic.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Tactic::Tactic(const World &world, bool active) : world(world), active_(active) {
}

Tactic::~Tactic() {
}

bool Tactic::done() const {
	// if this fails, then you probably have an active tactic that you forgot to implement done()
	// check to make sure that the signature of done() is correct, with the const at the back.
	assert(0);
}

void Tactic::set_player(Player::Ptr p) {
	if (player != p) {
		player = p;
		player_changed();
	}
}

std::string Tactic::description() const {
	return "no description";
}

void Tactic::player_changed() {
}

