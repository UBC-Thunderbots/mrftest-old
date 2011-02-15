#include "ai/hl/stp/skill/bump_to_goal.h"
#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/hl/stp/skill/kick.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;
using AI::HL::STP::SSM::SkillStateMachine;

namespace {
	class BumpToGoal : public Skill {
		private:
			void execute(const World &world, Player::Ptr player, const SkillStateMachine *, AI::HL::STP::Skill::Param &param, Context &context) const {
				// go after ball
				if (!player->has_ball()) {
					context.execute_after(go_to_ball());
					return;
				}

				// terminates once you are at the ball or you're ready to kick
				if ((player->position() - world.ball().position()).len() <= Robot::MAX_RADIUS || player->chicker_ready_time() == 0) {
					return;
				}
			}
	};

	BumpToGoal bump_to_goal_instance;
}

const Skill *AI::HL::STP::Skill::bump_to_goal() {
	return &bump_to_goal_instance;
}

