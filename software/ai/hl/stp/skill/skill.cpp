#include "ai/hl/stp/skill/skill.h"
#include "ai/flags.h"

using namespace AI::HL::STP::Skill;
using namespace AI::HL::W;

namespace {
	/**
	 * A Terminal state always transition to itself.
	 */
	class Terminal : public Skill {
		private:
			const Skill* execute(AI::HL::W::World&, AI::HL::W::Player::Ptr, const AI::HL::STP::SSM::SkillStateMachine*, Param&) const {
				return this;
			}
	};

	Terminal finish_instance;
	Terminal fail_instance;
}

Param::Param() : can_kick(true), move_flags(0), move_priority(AI::Flags::PRIO_MEDIUM) {
}

const Skill* AI::HL::STP::Skill::finish() {
	return &finish_instance;
}

const Skill* AI::HL::STP::Skill::fail() {
	return &fail_instance;
}

