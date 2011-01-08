#include "ai/hl/stp/tactic/tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Tactic::Tactic(World &world, bool active) : world(world), active_(active), ssm(NULL) {
}

Tactic::~Tactic() {
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

void Tactic::tick() {
	execute();

	// if no SSM, nothing to do
	if (ssm == NULL) return;

	// initialize skill if needed
	if (!skill) {
		skill = ssm->initial();
	}

	// execute the skill
	skill = skill->execute(world, player, ssm, param);
}

void Tactic::set_ssm(AI::HL::STP::SSM::SkillStateMachine* s) {
	if (s == ssm) return;
	ssm = s;
	skill = NULL;
}

void Tactic::player_changed() {
}

