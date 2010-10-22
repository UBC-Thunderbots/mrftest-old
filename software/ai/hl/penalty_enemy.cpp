#include "ai/hl/penalty_enemy.h"
#include "ai/hl/strategy.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/algorithm.h"
#include "util/dprint.h"

using AI::HL::PenaltyEnemy;
using namespace AI::HL::W;
using namespace AI::HL::Tactics;

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

void PenaltyEnemy::set_players(std::vector<Player::Ptr> p, Player::Ptr g) {
	if (!g.is()) {
		LOG_ERROR("no goalie");
	}
	if (p.size() == 0) {
		LOG_ERROR("no players to accompany goalie");
	}
	players = p;
	goalie = g;
}

void PenaltyEnemy::tick() {
/*
	unsigned int penaltyEnemy_flags = AI::Flags::calc_flags(world.playtype());
	for (unsigned int i = 0; i < player.size(); ++i) {
		move tactic(the_robots[i], world);
		tactic.set_position(standing_positions[i]);
		tactic.set_flags(flags);
		tactic.tick();
*/

	const Field &f = (world.field());
	const Point starting_position(-0.5 * f.length(), -0.5 * Robot::MAX_RADIUS);
	const Point ending_position(-0.5 * f.length(), 0.5 * Robot::MAX_RADIUS);

	Patrol(world, goalie, starting_position, ending_position, AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE);
}

