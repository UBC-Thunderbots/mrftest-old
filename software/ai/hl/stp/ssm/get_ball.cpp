#include "ai/hl/stp/ssm/get_ball.h"
#include "ai/hl/stp/skill/go_to_ball.h"

using namespace AI::HL::W;
using AI::HL::STP::SSM::SkillStateMachine;
using AI::HL::STP::Skill::Skill;

namespace {
	class GetBall : public SkillStateMachine {
		private:
			const Skill* initial() const {
				return AI::HL::STP::Skill::go_to_ball();
			}
	};

	GetBall get_ball_instance;
}

const SkillStateMachine* AI::HL::STP::SSM::get_ball() {
	return &get_ball_instance;
}

