#include "ai/flags.h"
#include "ai/hl/stp/action/intercept.h"

void AI::HL::STP::Action::intercept(AI::HL::STP::Player player, const Point target) {
	int counter = 0;
	player.type(AI::Flags::MoveType::INTERCEPT);
	player.dribble_slow();
	player.move(target, (target - player.position()).orientation(), Point());
	if (player.has_ball())
		counter++;
	else counter = 0;

	if (counter == 15) {
		std::printf("intercept complete");
		return;
	}
}
