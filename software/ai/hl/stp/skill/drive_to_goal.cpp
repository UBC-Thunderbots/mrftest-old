#include "ai/hl/stp/skill/drive_to_goal.h"
#include "ai/hl/stp/skill/go_to_ball.h"
#include "ai/hl/stp/skill/kick.h"
#include "ai/hl/stp/ssm/move_ball.h"
#include "ai/hl/stp/module/shoot.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

namespace {
	DriveToGoal drive_to_goal;
}

const DriveToGoal* DriveToGoal::instance() {
	return &drive_to_goal;
}

const Skill* DriveToGoal::execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, const AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const {

	// must have a ball
	if (!player->has_ball()) {
		return GoToBall::instance()->execute(world, player, ssm, param);
	}

	// TODO: active tactics always have high priority
	// override priority
	param.move_priority = AI::Flags::PRIO_HIGH;

	AI::HL::STP::Module::ShootStats shoot_stats = AI::HL::STP::Module::evaluate_shoot(world, player);

	// TODO
	if (shoot_stats.can_shoot && player->chicker_ready_time() == 0) {
		return Kick::instance()->execute(world, player, ssm, param);
	}

	// TODO
	// if can still dribble for some distance
	//   find somewhere more sensible
	//   move towards enemy goal

	return this;
}

