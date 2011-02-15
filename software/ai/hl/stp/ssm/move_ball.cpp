#include "ai/hl/stp/ssm/move_ball.h"
#include "ai/hl/stp/skill/drive_to_goal.h"

using namespace AI::HL::W;
using AI::HL::STP::SSM::SkillStateMachine;
using AI::HL::STP::Skill::Skill;

namespace {
	class MoveBall : public SkillStateMachine {
		private:
			const Skill *initial() const {
				return AI::HL::STP::Skill::drive_to_goal();
			}
	};

	MoveBall move_ball_instance;
}

const SkillStateMachine *AI::HL::STP::SSM::move_ball() {
	return &move_ball_instance;
}

