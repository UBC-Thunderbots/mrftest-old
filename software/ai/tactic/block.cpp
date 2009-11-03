#include "ai/tactic/block.h"

block::block(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player) : tactic(ball, field, team, player), move_tactic(new move(ball, field, team, player)) {
}

void block::set_target(player::ptr target) {
	the_target = target;
}		

void block::update() {
	const double GUARD_DIST = 10;
	point p = the_player->position() - the_target->position();

	if (p.cross(the_target->est_velocity()) == 0) {		
		// in the target's line of velocity
		const point UP(0,1);
		double p_side = p.cross(UP); 
		double v_side = the_target->est_velocity().cross(UP);

		if (p_side > 0 && v_side > 0) {
			// target is moving towards me, move towards him, but don't bump into him
			move_tactic->set_position(the_target->position() + the_target->est_velocity() * 1.0/GUARD_DIST);
		} else {
			// I am behind target, move towards where the target's moving for now
			move_tactic->set_position(the_target->position() + the_target->est_velocity());
		}
	} else {
		// move towards where the target's moving
		move_tactic->set_position(the_target->position() + the_target->est_velocity());
	}
	move_tactic->update();
}


