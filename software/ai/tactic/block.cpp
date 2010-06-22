#include "ai/tactic/block.h"
#include "ai/tactic/move.h"
#include "util/dprint.h"

block::block(player::ptr player, world::ptr world) : tactic(player), the_world(world) {
}

void block::set_target(robot::ptr target) {
	this->target = target;
}		

void block::tick() {
	if (!target){
		LOG_DEBUG(Glib::ustring::compose("%1 does not have a target to block!", the_player->name));
		move move_tactic(the_player, the_world);
		move_tactic.set_position(the_player->position());
		move_tactic.set_flags(flags);
		move_tactic.tick();
		return;
	}
	const field& the_field = the_world->field();
	LOG_DEBUG(Glib::ustring::compose("%1 aiming", the_player->name));
	const point goal = point(-the_field.length()/2.0, 0);
	point dir = target->position() - goal;
	point offset = dir.norm() * (2*robot::MAX_RADIUS);

	point dest = (dir - offset) + goal;
	move move_tactic(the_player, the_world);
	move_tactic.set_position(dest);
	move_tactic.set_orientation(dir.orientation());
	move_tactic.set_flags(flags);
	move_tactic.tick();
}

