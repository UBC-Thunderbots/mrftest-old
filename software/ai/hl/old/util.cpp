#include "ai/hl/old/util.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>

using namespace AI::HL::W;

void AI::HL::Util::waypoints_matching(const std::vector<Player::Ptr> &players, std::vector<Point> &waypoints) {
	// TODO: make more efficient
	std::vector<Point> locations;
	for (std::size_t i = 0; i < players.size(); ++i) {
		locations.push_back(players[i]->position());
	}
	const std::vector<Point> waypoints_orig = waypoints;
	std::vector<std::size_t> order = dist_matching(locations, waypoints);
	for (std::size_t i = 0; i < waypoints.size(); ++i) {
		waypoints[i] = waypoints_orig[order[i]];
	}
}

Player::Ptr AI::HL::Util::choose_best_pass(World &world, const std::vector<Player::Ptr> &friends) {
	double bestangle = 0;
	double bestdist = 1e99;
	Player::Ptr passee;
	for (size_t i = 0; i < friends.size(); ++i) {
		// see if this player is on line of sight
		if (!AI::HL::Util::can_receive(world, friends[i])) {
			continue;
		}
		// choose the most favourable distance
		const double dist = (friends[i]->position() - world.ball().position()).len();
		const double angle = AI::HL::Util::calc_best_shot(world, friends[i]).second;
		if (!passee.is() || angle > bestangle || (angle == bestangle && dist < bestdist)) {
			bestangle = angle;
			bestdist = dist;
			passee = friends[i];
		}
	}
	return passee;
}

