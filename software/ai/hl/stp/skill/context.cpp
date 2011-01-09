#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/ssm/ssm.h"
#include "util/dprint.h"

using namespace AI::HL::STP::Skill;
using AI::HL::STP::SSM::SkillStateMachine;
using namespace AI::HL::W;

Context::Context(World& w, Param& p) : world(w), param(p), ssm_(NULL), next_skill(NULL) {
}

const SkillStateMachine* Context::ssm() const {
	return ssm_;
}

void Context::set_player(Player::Ptr p) {
	player = p;
}

void Context::set_ssm(const AI::HL::STP::SSM::SkillStateMachine* ssm) {
	if (ssm != ssm_ ){
		ssm_ = ssm;
		if (ssm_ == NULL) {
			next_skill = NULL;
		} else {
			next_skill = ssm->initial();
		}
	}
}

void Context::reset_ssm() {
	if (ssm_ != NULL) {
		next_skill = ssm_->initial();
	}
}

void Context::run() {
	if (next_skill == NULL) {
		// ???
		return;
	}

	history.insert(next_skill);
	next_skill->execute(world, player, param, *this);
	history.clear();
}

void Context::transition(const Skill* skill) {
	next_skill = skill;
}

void Context::execute(const Skill* skill) {
	if (history.find(skill) != history.end()) {
		LOG_ERROR("skill loop!");
		return;
	}

	history.insert(skill);
	next_skill->execute(world, player, param, *this);
}

