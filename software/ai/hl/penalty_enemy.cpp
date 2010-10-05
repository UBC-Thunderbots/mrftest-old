#include "ai/hl/penalty_enemy.h"
#include "ai/hl/strategy.h"
//#include "ai/tactic/move.h"
//#include "ai/tactic/patrol.h"
#include <iostream>

using AI::HL::PenaltyEnemy;
using namespace AI::HL::W;



const double PenaltyEnemy::PENALTY_MARK_LENGTH = 0.45;

const double PenaltyEnemy::RESTRICTED_ZONE_LENGTH = 0.85;

const unsigned int PenaltyEnemy::NUMBER_OF_READY_POSITIONS;

/*The constructor*/
PenaltyEnemy::PenaltyEnemy(World &w) : world(w) {
	const Field &f = (world.field());

	/*Set robot positions*/
	ready_positions[0] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);
	ready_positions[1] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 2 * Robot::MAX_RADIUS);
	ready_positions[2] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -2 * Robot::MAX_RADIUS);
	ready_positions[3] = Point(-0.5 * f.length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);
}


void PenaltyEnemy::tick() {
/*
	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	for (unsigned int i = 0; i < the_robots.size(); ++i) {
		move tactic(the_robots[i], the_world);
		tactic.set_position(standing_positions[i]);
		tactic.set_flags(flags);
		tactic.tick();
*/
}


