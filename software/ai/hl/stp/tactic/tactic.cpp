#include "ai/hl/stp/tactic/tactic.h"

using namespace AI::HL::STP;
using namespace AI::HL::W;

Tactic::Tactic(World& world, double timeout) : world(world), timeout(timeout) {
	std::time(&start_time);
}

Tactic::~Tactic() {
}

bool Tactic::done() const {
	return true;
}

bool Tactic::timed_out() const {
	std::time_t curr_time;
	std::time(&curr_time);
	double diff = difftime(curr_time, start_time);
	return diff < timeout;
}

void Tactic::set_player(Player::Ptr p) {
	if (player != p) {
		player = p;
		player_changed();
	}
}

void Tactic::player_changed() {
}

