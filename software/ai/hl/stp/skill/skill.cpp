#include "ai/hl/stp/skill/skill.h"
#include "ai/flags.h"

using namespace AI::HL::STP::Skill;
using namespace AI::HL::W;

Param::Param() : can_kick(true), move_flags(0), move_type(AI::Flags::MOVE_NORMAL), move_priority(AI::Flags::PRIO_MEDIUM) {
}

//Skill::Skill(AI::HL::W::World& w, AI::HL::W::Player::Ptr p) : world(w), player(p) {
//}

