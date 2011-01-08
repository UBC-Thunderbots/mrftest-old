#include "ai/hl/stp/ssm/move_ball.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::SSM;
using AI::HL::STP::Skill::Skill;

namespace {
	MoveBall moveball;
}

const MoveBall* MoveBall::instance() {
	return &moveball;
}

const Skill* MoveBall::initial() const {
	// TODO
	return NULL;
}

