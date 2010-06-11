#include "ai/tactic/receive.h"
#include "ai/util.h"

receive::receive(player::ptr player, world::ptr world) : tactic(player), the_world(world), navi(player, world) {
}

void receive::tick() {
	if (!ai_util::can_pass(the_world, the_player)) {
		// try to find line of sight
		// for now just chase ball
		// TODO: implement, calculate rays and stuff
		navi.set_position(the_world->ball()->position());
	} else {
		// if this robot is to receive the pass, just stand still and turn towards the passer
		navi.set_orientation((the_world->ball()->position() - the_player->position()).orientation());
		navi.set_dribble();
	}
	navi.set_flags(flags);
	navi.tick();
}

