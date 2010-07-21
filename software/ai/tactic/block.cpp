#include "ai/tactic/block.h"
#include "ai/tactic/move.h"
#include "util/dprint.h"

#include "uicomponents/param.h"

namespace{
	DoubleParam BLOCK_ENEMY_DISTANCE("block: number of robot radii to stay away from enemy when blocking", 1.0, 0.0, 4.0);
}


Block::Block(Player::Ptr player, World::Ptr world) : Tactic(player), the_world(world) {
}

void Block::set_target(Robot::Ptr target) {
	this->target = target;
}		

void Block::tick() {
	if (!target){
		LOG_DEBUG(Glib::ustring::compose("%1 does not have a target to block!", player->name));
		Move move_tactic(player, the_world);
		move_tactic.set_position(player->position());
		move_tactic.set_flags(flags);
		move_tactic.tick();
		return;
	}
	const Field& the_field = the_world->field();
	LOG_DEBUG(Glib::ustring::compose("%1 aiming", player->name));
	const Point goal = Point(-the_field.length()/2.0, 0);
	Point dir = target->position() - goal;
	Point offset = dir.norm() * std::min((BLOCK_ENEMY_DISTANCE*Robot::MAX_RADIUS), std::max(dir.len() - Robot::MAX_RADIUS, 0.0));

	Point dest = (dir - offset) + goal;
	Move move_tactic(player, the_world);
	move_tactic.set_position(dest);
	move_tactic.set_orientation((-dir).orientation());
	move_tactic.set_flags(flags);
	move_tactic.tick();
}

