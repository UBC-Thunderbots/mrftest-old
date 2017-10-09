#include "ai/navigator/rrt_physics_planner.h"
// #include "ai/navigator/util.h"
#include "util/dprint.h"
#include "util/timestep.h"
#include "ai/navigator/util.h"
#include <cmath>

using namespace AI::Nav;
using namespace AI::Nav::W;
// using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;

namespace {
	// namespace Nav{
	constexpr double MAX_SPEED = 2.0;
	constexpr double THRESHOLD = 0.07;
// const double STEP_DISTANCE = 0.3;
	constexpr double TIMESTEP = 1.0 / TIMESTEPS_PER_SECOND;
	constexpr double VALID_REGION = 0.3 * (9 / 8) * TIMESTEP * TIMESTEP;
	// probability that we will take a step towards the goal
	constexpr double GOAL_PROB = 0.1;
	constexpr double WAYPOINT_PROB = 0.6;
	constexpr double RAND_PROB = 1.0 - GOAL_PROB - WAYPOINT_PROB;
	// number of iterations to go through for each robot until we give up and
	// just return the best partial path we've found
	constexpr int ITERATION_LIMIT = 1000;
	constexpr int NUM_WAYPOINTS = 50;
	constexpr bool POST_PROCESS = false;
}

std::vector<Point> AI::Nav::PhysicsPlanner::plan(Player player, Point goal, AI::Flags::MoveFlags added_flags) {
	curr_player = player;
	return rrt_plan(player, goal, POST_PROCESS, added_flags);
}

double PhysicsPlanner::distance(NodeTree<Point> *nearest, Point goal) {
	Point projected;
	Point currPlayerVelocity = curr_player.velocity();
	if (!nearest->parent()) {
		projected = nearest->data() + (currPlayerVelocity * TIMESTEP);
	} else {
		projected = 2 * nearest->data() - nearest->parent()->data();
	}

	return (goal - projected).len();
}

// extend by STEP_DISTANCE towards the target from the start
Point AI::Nav::PhysicsPlanner::extend(Player player, Glib::NodeTree<Point> *start, Point target) {
	Point projected;
	if (!start->parent()) {
		projected = start->data() + (player.velocity() * TIMESTEP);
	} else {
		projected = 2 * start->data() - start->parent()->data();
	}

	Point residual = (target - projected);
	Point normalizedDir = residual.norm();
	Point extendPoint;

	double maximumVel = std::sqrt(2 * Player::MAX_LINEAR_ACCELERATION * residual.len());
	if (maximumVel > Player::MAX_LINEAR_VELOCITY) {
		maximumVel = Player::MAX_LINEAR_VELOCITY;
	}

	extendPoint = normalizedDir * Player::MAX_LINEAR_ACCELERATION * TIMESTEP * TIMESTEP + projected;

	if ((extendPoint - start->data()).len() > maximumVel * TIMESTEP) {
		extendPoint = (extendPoint - start->data()).norm() * maximumVel * TIMESTEP + start->data();
	}

	// check if the point is invalid (collision, out of bounds, etc...)
	// if it is then return EmptyState()
	if (!AI::Nav::Util::valid_path(start->data(), extendPoint, world, player)) {
		return empty_state();
	}

	return extendPoint;
}

AI::Nav::PhysicsPlanner::PhysicsPlanner(World world) : RRTPlanner(world) {
}

