#include "ai/hl/stp/skill/skill.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/flags.h"

using namespace AI::HL::STP::Skill;
using namespace AI::HL::W;

Param::Param() : move_flags(0), move_priority(AI::Flags::PRIO_MEDIUM) {
}

