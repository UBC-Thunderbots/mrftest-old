#include "ai/hl/penalty_friendly.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/algorithm.h"
#include "util/dprint.h"

using AI::HL::PenaltyFriendly;
using namespace AI::HL::W;

const double PenaltyFriendly::PENALTY_MARK_LENGTH = 0.45;

const double PenaltyFriendly::RESTRICTED_ZONE_LENGTH = 0.85;

const unsigned int PenaltyFriendly::NUMBER_OF_READY_POSITIONS;

PenaltyFriendly::PenaltyFriendly(World &w) : world(w) {
	const Field &f = world.field();

	// Let the first robot to be always the shooter
	ready_positions[0] = Point(0.5 * f.length() - PENALTY_MARK_LENGTH - Robot::MAX_RADIUS, 0);

	ready_positions[1] = Point(0, 0);

	// Let two robots be on the offensive, in case there is a rebound
	ready_positions[2] = Point(0.5 * f.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);
	ready_positions[3] = Point(0.5 * f.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);
}


void PenaltyFriendly::tick() {
	if (players.size() == 0) {
		LOG_WARN("no robots");
		return;
	}
	if (world.playtype() == PlayType::PREPARE_PENALTY_FRIENDLY) {
		for (size_t i = 0; i < players.size(); ++i) {
			// move the robots to position
			players[i]->move(ready_positions[i], (ready_positions[i] - players[i]->position()).orientation(), AI::Flags::FLAG_PENALTY_KICK_FRIENDLY, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
		}
	} else if (world.playtype() == PlayType::EXECUTE_PENALTY_FRIENDLY) {
		// make shooter shoot
		const Player::Ptr shooter = players[0];
		AI::HL::Tactics::shoot(world, shooter, AI::Flags::FLAG_CLIP_PLAY_AREA);

		for (size_t i = 1; i < players.size(); ++i) {
			players[i]->move(ready_positions[i], (ready_positions[i] - players[i]->position()).orientation(), AI::Flags::FLAG_PENALTY_KICK_FRIENDLY, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
		}
	} else {
		LOG_WARN("penalty_friendly: unhandled playtype");
		return;
	}
}

