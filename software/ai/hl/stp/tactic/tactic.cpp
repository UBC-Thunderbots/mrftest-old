#include "ai/hl/stp/tactic/tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Tactic::Tactic(World &world, bool active, double timeout) : world(world), active_(active), timeout_(timeout) {
	std::time(&start_time);
}

Tactic::~Tactic() {
}

double Tactic::elapsed_time() const {
	std::time_t curr_time;
	std::time(&curr_time);
	return difftime(curr_time, start_time);
}

bool Tactic::done() const {
	return true;
}

bool Tactic::active() const {
	return active_;
}

void Tactic::set_player(Player::Ptr p) {
	if (player != p) {
		player = p;
		player_changed();
	}
}

void Tactic::player_changed() {
}

