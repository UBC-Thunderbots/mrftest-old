#include "ai/navigator/util.h"
#include "util/dprint.h"
#include "util/timestep.h"
#include "ai/navigator/rrt_base.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::RRT;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;


namespace{

	const double MAX_SPEED = 2.0;
	const double THRESHOLD = 0.07;
// const double STEP_DISTANCE = 0.3;
	const double TIMESTEP = 1.0 / static_cast<double>(TIMESTEPS_PER_SECOND);
	const double VALID_REGION = 0.3 * (9 / 8) * TIMESTEP * TIMESTEP;
	// probability that we will take a step towards the goal
	const double GOAL_PROB = 0.1;
	const double WAYPOINT_PROB = 0.6;
	const double RAND_PROB = 1.0 - GOAL_PROB - WAYPOINT_PROB;
	// number of iterations to go through for each robot until we give up and
	// just return the best partial path we've found
	const int ITERATION_LIMIT = 1000;
	const int NUM_WAYPOINTS = 50;

	class RRTPhysicsNavigator : public RRTBase {
		public:
			NavigatorFactory &factory() const;
			void tick();
			static Navigator::Ptr create(World &world);

		private:
			RRTPhysicsNavigator(World &world);
			~RRTPhysicsNavigator();

			Waypoints::Ptr currPlayerWaypoints;
			Point currPlayerVelocity;

			double distance(NodeTree<Point> *nearest, Point goal);
			Point extend(Player::Ptr player, Point projected, Point start, Point target);

	};

	class RRTPhysicsNavigatorFactory : public NavigatorFactory {
		public:
			RRTPhysicsNavigatorFactory();
			~RRTPhysicsNavigatorFactory();
			Navigator::Ptr create_navigator(World &world) const;
	};

	RRTPhysicsNavigatorFactory factory_instance;

	NavigatorFactory &RRTPhysicsNavigator::factory() const {
		return factory_instance;
	}

	void RRTPhysicsNavigator::tick() {
		struct timespec currentTime;
		Player::Path path;
		std::vector<Point> pathPoints;

		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			path.clear();
			Player::Ptr player = world.friendly_team().get(i);
			currentTime = world.monotonic_time();

			const double dist = (player->position() - player->destination().first).len();
			struct timespec timeToAdd = double_to_timespec(dist / MAX_SPEED);
			struct timespec finalTime;

			timespec_add(currentTime, timeToAdd, finalTime);

			currPlayerWaypoints = Waypoints::Ptr::cast_dynamic(player->object_store()[typeid(*this)]);
			currPlayerVelocity = player->velocity();

			pathPoints.clear();
			pathPoints = rrt_plan(player, player->destination().first, false);

			double destOrientation = player->destination().second;
			for (std::size_t j = 0; j < pathPoints.size(); ++j) {
				// the last point will just use whatever the last orientation was
				if (j + 1 != pathPoints.size()) {
					destOrientation = (pathPoints[j + 1] - pathPoints[j]).orientation();
				}

				path.push_back(std::make_pair(std::make_pair(pathPoints[j], destOrientation), finalTime));
			}

			// just use the current player position as the destination if we are within the
			// threshold already
			if (pathPoints.size() == 0) {
				path.push_back(std::make_pair(std::make_pair(player->position(), destOrientation), finalTime));
			}

			player->path(path);
		}
	}

	double RRTPhysicsNavigator::distance(NodeTree<Point> *nearest, Point goal) {
		Point projected;
		if (!nearest->parent()) {
			projected = nearest->data() + (currPlayerVelocity * TIMESTEP);
		} else {
			projected = 2 * nearest->data() - nearest->parent()->data();
		}

		return (goal - projected).len();
	}

	// extend by STEP_DISTANCE towards the target from the start
	Point RRTPhysicsNavigator::extend(Player::Ptr player, Point projected, Point start, Point target) {
		Point residual = (target - projected);
		Point normalizedDir = residual.norm();
		Point extendPoint;

		double maximumVel = sqrt(2 * Player::MAX_LINEAR_ACCELERATION * residual.len());
		if (maximumVel > Player::MAX_LINEAR_VELOCITY) {
			maximumVel = Player::MAX_LINEAR_VELOCITY;
		}

		extendPoint = normalizedDir * Player::MAX_LINEAR_ACCELERATION * TIMESTEP * TIMESTEP + projected;

		if ((extendPoint - start).len() > maximumVel * TIMESTEP) {
			extendPoint = (extendPoint - start).norm() * maximumVel * TIMESTEP + start;
		}

		// check if the point is invalid (collision, out of bounds, etc...)
		// if it is then return EmptyState()
		if (!valid_path(start, extendPoint, world, player)) {
			return empty_state();
		}

		return extendPoint;
	}

	Navigator::Ptr RRTPhysicsNavigator::create(World &world) {
		const Navigator::Ptr p(new RRTPhysicsNavigator(world));
		return p;
	}

	RRTPhysicsNavigator::RRTPhysicsNavigator(World &world) : RRTBase(world) {
	}

	RRTPhysicsNavigator::~RRTPhysicsNavigator() {
	}

	RRTPhysicsNavigatorFactory::RRTPhysicsNavigatorFactory() : NavigatorFactory("RRT Physics Navigator") {
	}

	RRTPhysicsNavigatorFactory::~RRTPhysicsNavigatorFactory() {
	}

	Navigator::Ptr RRTPhysicsNavigatorFactory::create_navigator(World &world) const {
		return RRTPhysicsNavigator::create(world);
	}

}

