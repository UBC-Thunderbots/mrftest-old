#include "ai/hl/stp/skill/spin_at_ball.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;
using AI::HL::STP::SSM::SkillStateMachine;

namespace {
	class SpinAtBall : public Skill {
		private:
			void execute(const World &world, Player::Ptr player, const SkillStateMachine *, Param &param, Context &context) const {
				// TODO
			}
	};

	SpinAtBall spin_at_ball_instance;
}

const Skill *spin_at_ball() {
	return &spin_at_ball_instance;
}

