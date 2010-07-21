#include "ai/role/penalty_enemy.h"
#include "ai/tactic/move.h"
#include "ai/tactic/patrol.h"

#include <iostream>

PenaltyEnemy::PenaltyEnemy(World::Ptr world) : the_world(world) {
	const Field& the_field(the_world->field());

	standing_positions[0] = Point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);
	standing_positions[1] = Point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 2 * Robot::MAX_RADIUS);
	standing_positions[2] = Point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -2 * Robot::MAX_RADIUS);
	standing_positions[3] = Point(-0.5 * the_field.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);
}

void PenaltyEnemy::tick() {
	unsigned int flags = AIFlags::calc_flags(the_world->playtype());
	for (unsigned int i = 0; i < players.size(); ++i) {
		Move tactic(players[i], the_world);
		tactic.set_position(standing_positions[i]);
		tactic.set_flags(flags);
		tactic.tick();
	}
}

void PenaltyEnemy::players_changed() {
}

PenaltyGoalie::PenaltyGoalie(World::Ptr world) : the_world(world) {
}

void PenaltyGoalie::tick() {
	if (players.size() != 1) {
		std::cerr << "penalty_enemy: we only want 1 goalie!" << std::endl;
	}

	if (players.size() == 0) return;

	const Field& f = the_world->field();
	const Point starting_position(-0.5 * f.length(), - 0.5 * Robot::MAX_RADIUS);
	const Point ending_position(-0.5 * f.length(), 0.5 * Robot::MAX_RADIUS);

	unsigned int flags = 0;
	Patrol tactic(players[0], the_world, flags, starting_position, ending_position);
	tactic.tick();
}

void PenaltyGoalie::players_changed() {
}

