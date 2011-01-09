#include "ai/hl/stp/skill/skill.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/ssm/ssm.h"
#include "ai/flags.h"

using namespace AI::HL::STP::Skill;
using namespace AI::HL::W;

namespace {

	/**
	 * A Terminal state always transition to itself.
	 */
	class Terminal : public Skill {
		private:
			void execute(World&, Player::Ptr, Param&, Context&) const {
			}
	};

	Terminal finish_instance;
	Terminal fail_instance;
}

Param::Param() : move_flags(0), move_priority(AI::Flags::PRIO_MEDIUM) {
}

const Skill* AI::HL::STP::Skill::finish() {
	return &finish_instance;
}

const Skill* AI::HL::STP::Skill::fail() {
	return &fail_instance;
}

