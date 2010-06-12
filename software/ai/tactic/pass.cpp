#include "ai/tactic/pass.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/kick.h"
#include "ai/tactic/move.h"
#include "ai/util.h"

#include <iostream>

pass::pass(player::ptr player, world::ptr world, player::ptr receiver) : tactic(player), the_world(world), the_receiver(receiver) {
}

void pass::tick() {
	if (!ai_util::has_ball(the_player) || !the_player->sense_ball()) {
		chase chase_tactic(the_player, the_world);
		chase_tactic.set_flags(flags);
		chase_tactic.tick();
		/*
		chase_and_shoot tactic(the_player, the_world);
		tactic.set_target(the_receiver->position());
		tactic.set_flags(flags);
		tactic.tick();
		*/
		return;
	}

	bool should_pass = ai_util::can_pass(the_world, the_receiver);
	// do we need this velocity threshold?
	// && the_receiver->est_velocity().len() < ai_util::VEL_CLOSE;

	if (should_pass) {
		// std::cout << " pass: let's shoot " << std::endl;
		kick kick_tactic(the_player, the_world);
		kick_tactic.set_target(the_receiver->position());
		kick_tactic.set_flags(flags);
		kick_tactic.tick();
	} else {
		// std::cout << " pass: move to receiver " << std::endl;
		move move_tactic(the_player, the_world);
		move_tactic.set_position(the_receiver->position());
		// ALSO FACE TOWARDS RECEIVER
		const point diff = the_receiver->position() - the_player->position();
		move_tactic.set_orientation(diff.orientation());
		move_tactic.set_flags(flags);
		move_tactic.tick();
	}
}

