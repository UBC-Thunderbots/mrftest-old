#include "ai/hl/stp/skill/spin_at_ball.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

namespace {
	SpinAtBall spin_at_ball;
}

const SpinAtBall* SpinAtBall::instance() {
	return &spin_at_ball;
}

const Skill* SpinAtBall::execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const {

	// TODO
	
	return this;
}

