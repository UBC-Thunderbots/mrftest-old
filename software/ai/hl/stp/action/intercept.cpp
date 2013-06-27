#include "ai/flags.h"
#include "ai/hl/stp/action/intercept.h"

void AI::HL::STP::Action::intercept(AI::HL::STP::Player player, const Point target) {
	player.type(AI::Flags::MoveType::INTERCEPT);
	player.dribble_slow();
	player.move(target, (target - player.position()).orientation(), Point());
}
