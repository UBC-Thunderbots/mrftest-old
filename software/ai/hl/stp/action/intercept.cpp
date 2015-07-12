#include "ai/flags.h"
#include "ai/hl/stp/action/intercept.h"

void AI::HL::STP::Action::intercept(AI::HL::STP::Player player, const Point target) {
	player.type(AI::Flags::MoveType::INTERCEPT);
	//slow down the dribbler to make it easier to catch the ball
	player.dribble(AI::BE::Player::DribbleMode::CATCH);
	player.move(target, (target - player.position()).orientation(), Point());
}



