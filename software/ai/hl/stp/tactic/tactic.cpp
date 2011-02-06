#include "ai/hl/stp/tactic/tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Tactic::Tactic(World &world, bool active) : world(world), active_(active), context(world, param), ssm(NULL) {
}

Tactic::~Tactic() {
}

bool Tactic::done() const {
	return true;
}

void Tactic::set_player(Player::Ptr p) {
	if (player != p) {
		player = p;
		context.set_player(player);
		context.set_ssm(NULL);
		initialize();
	}
}

void Tactic::set_move_flags(unsigned int f) {
	// TODO: check if param is not NULL
	param.move_flags = f;
}

void Tactic::tick() {
	const AI::HL::STP::SSM::SkillStateMachine* prev_ssm = ssm;
	ssm = NULL;

	execute();

	// if no SSM, nothing to do
	if (ssm == NULL) return;

	// reset skill if ssm is changed
	if (ssm != prev_ssm) {
		context.set_ssm(ssm);
	}

	context.run();
}

void Tactic::initialize() {
}

void Tactic::set_ssm(const AI::HL::STP::SSM::SkillStateMachine* s) {
	ssm = s;
}

bool Tactic::ssm_done() const {
	return context.done();
}

