#include "ai/tactic/receive.h"
#include "ai/util.h"
#include "ai/flags.h"

Receive::Receive(Player::ptr player, World::ptr world) : Tactic(player), the_world(world), navi(player, world) {
}

void Receive::tick() {
	if (!AIUtil::can_receive(the_world, player)) {
		// TODO: maybe make the robot face towards the ball first.
		// TODO: implement, calculate rays and stuff and calculate the best position. this will be now, buz else, we will never truely receive the ball!!
		navi.set_position(the_world->ball()->position());
	} else {
		// navigator always turn towards ball by default
		// all we need now is to turn on dribbler
		navi.set_dribbler();
	}
	navi.set_flags(flags | AIFlags::CLIP_PLAY_AREA);
	navi.tick();
}

