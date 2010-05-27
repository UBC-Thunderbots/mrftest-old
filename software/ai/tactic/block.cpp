#include "ai/tactic/block.h"

block::block(player::ptr player, world::ptr world) : tactic(player), navi(player, world) {
}

void block::set_target(robot::ptr target) {
	this->target = target;
}		

void block::tick() {
	const double GUARD_DIST = 20;
	point p = the_player->position() - target->position();

	if (p.cross(target->est_velocity()) == 0) {		
		// in the target's line of velocity
		const point UP(0,1);
		double p_side = p.cross(UP); 
		double v_side = target->est_velocity().cross(UP);

		if (p_side > 0 && v_side > 0) {
			// target is moving towards me, move towards him, but don't bump into him
			navi.set_position(target->position() + target->est_velocity() * 1.0/GUARD_DIST);
		} else {
			// I am behind target, move towards where the target's moving for now
			navi.set_position(target->position() + target->est_velocity());
		}
	} else {
		// move towards where the target's moving
		navi.set_position(target->position() + target->est_velocity());
	}
	navi.set_flags(flags);
	navi.tick();
}

