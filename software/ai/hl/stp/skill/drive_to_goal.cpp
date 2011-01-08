#include "ai/hl/stp/skill/drive_to_goal.h"
#include "ai/hl/stp/ssm/move_ball.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

const Skill* DriveToGoal::execute(AI::HL::W::World& world, AI::HL::W::Player::Ptr player, AI::HL::STP::SSM::SkillStateMachine* ssm, Param& param) const {
	// TODO
	// if has_ball
	//   if can_shoot
	//   transition to kick
	// else if ball_on_front and ball_visible
	//   transition to gotoball
	//
	// move towards enemy goal
	// repeat
	return this;
}

