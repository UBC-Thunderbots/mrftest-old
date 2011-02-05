#include "ai/hl/stp/skill/drive_to_goal.h"
#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/hl/stp/skill/kick.h"
#include "ai/hl/stp/skill/context.h"
#include "ai/hl/stp/evaluate/shoot.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;
using AI::HL::STP::SSM::SkillStateMachine;

namespace {
	class DriveToGoal : public Skill {
		private:
			void execute(World& world, Player::Ptr player, const SkillStateMachine*, AI::HL::STP::Skill::Param& param, Context& context) const {
				// must have a ball
				if (!player->has_ball()) {
					context.execute_after(go_to_ball());
					return;
				}

				AI::HL::STP::Evaluation::ShootStats shoot_stats = AI::HL::STP::Evaluation::shoot_stats(world, player);

				// TODO
				if (shoot_stats.can_shoot && player->chicker_ready_time() == 0) {
					context.execute_after(kick());
					return;
				}

				// TODO
				// if can still dribble for some distance
				//   find somewhere more sensible
				//   e.g. move towards enemy goal
			}
	};

	DriveToGoal drive_to_goal_instance;
}

const Skill* AI::HL::STP::Skill::drive_to_goal() {
	return &drive_to_goal_instance;
}

