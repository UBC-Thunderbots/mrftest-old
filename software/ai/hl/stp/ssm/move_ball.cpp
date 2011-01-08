#include "ai/hl/stp/ssm/move_ball.h"
#include "ai/hl/stp/skill/drive_to_goal.h"

using namespace AI::HL::W;
using AI::HL::STP::SSM::MoveBall;

namespace {
	MoveBall move_ball;
}

const MoveBall* MoveBall::instance() {
	return &move_ball;
}

const AI::HL::STP::Skill::Skill* MoveBall::initial() const {
	return AI::HL::STP::Skill::DriveToGoal::instance();
}

