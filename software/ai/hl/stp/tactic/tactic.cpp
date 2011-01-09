#include "ai/hl/stp/tactic/tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Tactic::Tactic(World &world, bool active) : world(world), move_flags(0), active_(active), ssm(NULL) {
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

void Tactic::set_move_flags(unsigned int f) {
	move_flags = f;
}

void Tactic::tick() {
	const AI::HL::STP::SSM::SkillStateMachine* prev_ssm = ssm;
	ssm = NULL;

	execute();

	// if no SSM, nothing to do
	if (ssm == NULL) return;

	// reset skill if ssm is changed
	if (ssm != prev_ssm) {
		skill = NULL;
	}

	// initialize skill if needed
	if (!skill) {
		skill = ssm->initial();
	}

	// execute the skill
	skill = skill->execute(world, player, ssm, param);
}

void Tactic::set_ssm(const AI::HL::STP::SSM::SkillStateMachine* s) {
	ssm = s;
}

void Tactic::player_changed() {
}

