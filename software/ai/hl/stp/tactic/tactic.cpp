#include "ai/hl/stp/tactic/tactic.h"
#include "util/param.h"
#include "util/dprint.h"
#include <stdexcept>

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Tactic::~Tactic() = default;

bool Tactic::done() const {
	// if this fails, then you probably have an active tactic that you forgot to implement done()
	// check to make sure that the signature of done() is correct, with the const at the back.
	throw std::logic_error("Active tactic's function [bool Tactic::done() const] not overridden.");
}

bool Tactic::fail() const {
	return false;
}

Player Tactic::get_player() const {
	return player;
}

void Tactic::set_player(Player p) {
	if (player != p) {
		player = p;
		player_changed();
	}
}

void Tactic::draw_overlay(Cairo::RefPtr<Cairo::Context>) const {
}

Tactic::Tactic(World world, bool active) : world(world), active_(active) {
}

void Tactic::player_changed() {
}

