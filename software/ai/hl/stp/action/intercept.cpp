#include "ai/flags.h"
#include "ai/hl/stp/action/intercept.h"

void AI::HL::STP::Action::intercept(AI::HL::STP::Player::Ptr player, const Point target) {
	player->type(AI::Flags::MoveType::INTERCEPT);
	player->move(target, (target - player->position()).orientation(), Point());
}
