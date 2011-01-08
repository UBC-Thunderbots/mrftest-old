#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/hl/stp/ssm/ssm.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

namespace {
	GoToBall drive_to_goal;
}

const GoToBall* GoToBall::instance() {
	return &drive_to_goal;
}

const Skill* GoToBall::execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const {

	if (player->has_ball()) {
		// umm not sure what to do next
		return ssm->initial()->execute(world, player, ssm, param);
	}

	// override priority
	player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);

	return this;
}

