#include "ai/hl/penalty_friendly.h"

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
}

