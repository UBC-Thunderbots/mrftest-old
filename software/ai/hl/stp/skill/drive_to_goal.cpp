#include "ai/hl/stp/skill/drive_to_goal.h"
#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/hl/stp/skill/kick.h"
#include "ai/hl/stp/ssm/move_ball.h"
#include "ai/hl/stp/evaluate/shoot.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

namespace {
	class DriveToGoal : public Skill {
		private:
			const Skill* execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const {
				// must have a ball
				if (!player->has_ball()) {
					return go_to_ball()->execute(world, player, ssm, param);
				}

				AI::HL::STP::Evaluate::ShootStats shoot_stats = AI::HL::STP::Evaluate::shoot_stats(world, player);

				// TODO
				if (shoot_stats.can_shoot && player->chicker_ready_time() == 0) {
					return kick()->execute(world, player, ssm, param);
				}

				// TODO
				// if can still dribble for some distance
				//   find somewhere more sensible
				//   e.g. move towards enemy goal

				return this;
			}
	};

	DriveToGoal drive_to_goal_instance;
}

const Skill* AI::HL::STP::Skill::drive_to_goal() {
	return &drive_to_goal_instance;
}

