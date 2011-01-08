#include "ai/hl/stp/skill/drive_to_goal.h"
#include "ai/hl/stp/skill/kick.h"
#include "ai/hl/stp/ssm/move_ball.h"
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

	// TODO: active tactics always have high priority
	// override priority
	param.move_priority = AI::Flags::PRIO_HIGH;

	// TODO
	if (player->has_ball()) {
		return Kick::instance();
	} else {
		// else if ball_on_front and ball_visible
		//   transition to gotoball
	}

	// TODO: find somewhere more sensible
	// move towards enemy goal

	return this;
}

