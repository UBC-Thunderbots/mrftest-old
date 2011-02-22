#include "ai/hl/stp/tactic/tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Tactic::Tactic(const World &world, bool active) : world(world), active_(active), context(world, param), ssm(0) {
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
		context.set_ssm(0);
		initialize();
	}
}

void Tactic::tick() {
	const AI::HL::STP::SSM::SkillStateMachine *prev_ssm = ssm;
	ssm = 0;

	execute();

	// if no SSM, nothing to do
	if (!ssm) {
		return;
	}

	// reset skill if ssm is changed
	if (ssm != prev_ssm) {
		context.set_ssm(ssm);
	}

	context.run();
}

std::string Tactic::description() const {
	return "";
}

void Tactic::initialize() {
}

void Tactic::set_ssm(const AI::HL::STP::SSM::SkillStateMachine *s) {
	ssm = s;
}

bool Tactic::ssm_done() const {
	return context.done();
}

