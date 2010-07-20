#include "ai/role/prepare_penalty_enemy.h"

PreparePenaltyEnemy::PreparePenaltyEnemy(RefPtr<World> world) : the_world(world) {
	const Field& the_field(the_world->field());

	standing_positions[0] = Point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);
	standing_positions[1] = Point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 2 * Robot::MAX_RADIUS);
	standing_positions[2] = Point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -2 * Robot::MAX_RADIUS);
	standing_positions[3] = Point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);
}

void PreparePenaltyEnemy::tick(){
	unsigned int flags = AIFlags::calc_flags(the_world->playtype());
	for (unsigned int i = 0; i < the_tactics.size(); ++i) {
		the_tactics[i]->set_flags(flags);
		the_tactics[i]->tick();
	}
}

void PreparePenaltyEnemy::robots_changed() {
	the_tactics.clear();

	for (unsigned int i = 0; i < robots.size(); ++i) {
		if (i < NUM_POSITIONS) {
			RefPtr<Move> tactic(new Move(robots[i], the_world));
			tactic->set_position(standing_positions[i]);	
			the_tactics.push_back(tactic);
		}
	}	
}

