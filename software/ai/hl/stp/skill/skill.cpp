#include "ai/hl/stp/skill/skill.h"
#include "ai/flags.h"

using namespace AI::HL::STP::Skill;
using namespace AI::HL::W;

namespace {
	Terminal finish_instance;
	Terminal fail_instance;
}

Param::Param() : can_kick(true), move_flags(0), move_priority(AI::Flags::PRIO_MEDIUM) {
}

const Terminal* Terminal::finish() {
	return &finish_instance;
}

const Terminal* Terminal::fail() {
	return &fail_instance;
}

const Skill* Terminal::execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const {
	return this;
}

