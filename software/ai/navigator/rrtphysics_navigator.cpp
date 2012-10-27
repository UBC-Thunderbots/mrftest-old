#include "ai/navigator/util.h"
#include "util/dprint.h"
#include "util/timestep.h"
#include "ai/navigator/rrt_physics_planner.h"
#include "ai/navigator/navigator.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;

namespace {
	const double MAX_SPEED = 2.0;
	const double THRESHOLD = 0.07;
// const double STEP_DISTANCE = 0.3;
	const double TIMESTEP = 1.0 / static_cast<double>(TIMESTEPS_PER_SECOND);
	const double VALID_REGION = 0.3 * (9 / 8) * TIMESTEP * TIMESTEP;
	// probability that we will take a step towards the goal
	const double GOAL_PROB = 0.1;
	const double WAYPOINT_PROB = 0.6;
	const double RAND_PROB = 1.0 - GOAL_PROB - WAYPOINT_PROB;


	class RRTPhysicsNavigator : public Navigator {
		public:
			RRTPhysicsNavigator(World world);
			void tick();
			NavigatorFactory &factory() const;

		private:
			PhysicsPlanner planner;
	};
}

RRTPhysicsNavigator::RRTPhysicsNavigator(World world) : Navigator(world), planner(world) {
}

void RRTPhysicsNavigator::tick() {
	struct timespec currentTime;
	Player::Path path;
	std::vector<Point> pathPoints;

	for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
		path.clear();
		Player player = world.friendly_team().get(i);
		currentTime = world.monotonic_time();
		const double dist = (player.position() - player.destination().first).len();
		struct timespec timeToAdd = double_to_timespec(dist / MAX_SPEED);
		struct timespec finalTime;

		timespec_add(currentTime, timeToAdd, finalTime);
		pathPoints.clear();
		pathPoints = planner.plan(player, player.destination().first);

		Angle destOrientation = player.destination().second;
		for (std::size_t j = 0; j < pathPoints.size(); ++j) {
			// the last point will just use whatever the last orientation was
			if (j + 1 != pathPoints.size()) {
				destOrientation = (pathPoints[j + 1] - pathPoints[j]).orientation();
			}

			path.push_back(std::make_pair(std::make_pair(pathPoints[j], destOrientation), finalTime));
		}

		// just use the current player position as the destination if we are within the
		// threshold already
		if (pathPoints.empty()) {
			path.push_back(std::make_pair(std::make_pair(player.position(), destOrientation), finalTime));
		}

		player.path(path);
	}
}

NAVIGATOR_REGISTER(RRTPhysicsNavigator)

