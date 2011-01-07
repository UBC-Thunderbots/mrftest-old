#include "ai/hl/stp/skill/drive_to_goal.h"

using namespace AI::HL::STP::Skill;
using namespace AI::HL::W;

DriveToGoal::DriveToGoal(AI::HL::W::World& w, AI::HL::W::Player::Ptr p) : Skill(w, p) {
}

void DriveToGoal::run() {
	// if has_ball
	//   if can_shoot
	//   transition to kick
	// else if ball_on_front and ball_visible
	//   transition to gotoball
	//
	// move towards enemy goal
	// repeat
}

