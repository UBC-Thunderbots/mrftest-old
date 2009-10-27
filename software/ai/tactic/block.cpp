#include "ai/tactic/block.h"

block::block(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), move_tactic(new move(ball, field, team, player)) {
}

void block::set_target(player::ptr target) {
	the_target = target;
}		

void block::update() {
	if ((the_player->position() - the_target->position()).cross(the_target->est_velocity()) == 0) {
		// in the target's way
	} else {
		move_tactic->set_position(the_target->position() + the_target->est_velocity());
		// set orientation
		move_tactic->update();
	}

}


