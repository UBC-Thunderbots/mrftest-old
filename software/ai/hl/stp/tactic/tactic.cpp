#include "ai/hl/stp/tactic/tactic.h"
#include "util/param.h"
#include "util/dprint.h"
#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;


BoolParam enable_tactic_logging("Enable Tactic Log", "AI", true);

Tactic::Tactic(const World &world, bool active) : world(world), active_(active) {
	if(enable_tactic_logging){
		LOG_DEBUG(description());
	}
}

bool Tactic::done() const {
	// if this fails, then you probably have an active tactic that you forgot to implement done()
	// check to make sure that the signature of done() is correct, with the const at the back.
	assert(0);
}

bool Tactic::fail() const {
	return false;
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

void Tactic::draw_overlay(Cairo::RefPtr<Cairo::Context>) const {
	// do nothing..
}

void Tactic::player_changed() {
}

