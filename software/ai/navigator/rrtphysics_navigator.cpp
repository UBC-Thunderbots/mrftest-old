#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "util/dprint.h"
#include "util/objectstore.h"
#include "util/timestep.h"
#include <glibmm.h>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
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
	// number of iterations to go through for each robot until we give up and
	// just return the best partial path we've found
	const int ITERATION_LIMIT = 1000;
	const int NUM_WAYPOINTS = 50;

	class Waypoints : public ObjectStore::Element {
		public:
			typedef ::RefPtr<Waypoints> Ptr;

			Point points[NUM_WAYPOINTS];
	};

	class RRTPhysicsNavigator : public Navigator {
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
			Point random_point();
			Point choose_target(Point goal);
			NodeTree<Point> *nearest(NodeTree<Point> *tree, Point target);
			Point empty_state();
			Point extend(Player::Ptr player, Point projected, Point start, Point target);
			bool is_empty_state(Point toCheck);
			std::deque<Point> rrt_plan(Player::Ptr player, Point initial, Point goal);
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
		std::vector<std::pair<std::pair<Point, double>, timespec> > path;
		std::deque<Point> pathPoints;

		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			path.clear();
			Player::Ptr player = world.friendly_team().get(i);
			currentTime = world.monotonic_time();

			const double dist = (player->position() - player->destination().first).len();
			struct timespec timeToAdd = double_to_timespec(dist / MAX_SPEED);
			struct timespec finalTime;

			timespec_add(currentTime, timeToAdd, finalTime);

			// create new waypoints for the player if they have not been created yet
			if (!player->object_store()[typeid(*this)].is()) {
				Waypoints::Ptr newWaypoints(new Waypoints);
				player->object_store()[typeid(*this)] = newWaypoints;
			}

			currPlayerWaypoints = Waypoints::Ptr::cast_dynamic(player->object_store()[typeid(*this)]);
			currPlayerVelocity = player->velocity();

			pathPoints.clear();
			pathPoints = rrt_plan(player, player->position(), player->destination().first);

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
		if (nearest->parent() == NULL) {
			projected = nearest->data() + (currPlayerVelocity * TIMESTEP);
		} else {
			projected = 2 * nearest->data() - nearest->parent()->data();
		}

		return (goal - projected).len();
	}

	Point RRTPhysicsNavigator::empty_state() {
		return Point(-10000, -10000);
	}

	bool RRTPhysicsNavigator::is_empty_state(Point toCheck) {
		return toCheck.x == empty_state().x && toCheck.y && empty_state().y;
	}

	// generate a random point from the field
	Point RRTPhysicsNavigator::random_point() {
		double randomX = ((std::rand() % static_cast<int>(world.field().length() * 100)) - (world.field().length() * 50)) / 100;
		double randomY = ((std::rand() % static_cast<int>(world.field().width() * 100)) - (world.field().width() * 50)) / 100;

		return Point(randomX, randomY);
	}

	// choose a target to extend toward, the goal, a waypoint or a random point
	Point RRTPhysicsNavigator::choose_target(Point goal) {
		double p = std::rand() / double(RAND_MAX);
		int i = static_cast<int>(std::rand() % NUM_WAYPOINTS);

		if (p > 0 && p <= WAYPOINT_PROB) {
			return currPlayerWaypoints->points[i];
		} else if (p > WAYPOINT_PROB && p < (WAYPOINT_PROB + RAND_PROB)) {
			return random_point();
		} else {
			return goal;
		}
	}

	// finds the point in the tree that is nearest to the target point
	NodeTree<Point> *RRTPhysicsNavigator::nearest(NodeTree<Point> *rrtTree, Point target) {
		NodeTree<Point> *nearest = rrtTree;
		NodeTree<Point> *currNode;

		std::vector<NodeTree<Point> *> nodeQueue;
		nodeQueue.push_back(rrtTree);

		// iterate through all the nodes in the tree, finding which is closest to the target
		while (nodeQueue.size() > 0) {
			currNode = nodeQueue.back();
			nodeQueue.pop_back();

			if (distance(currNode, target) < distance(nearest, target)) {
				nearest = currNode;
			}

			for (unsigned int i = 0; i < currNode->child_count(); ++i) {
				nodeQueue.push_back(currNode->nth_child(i));
			}
		}

		return nearest;
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

	std::deque<Point> RRTPhysicsNavigator::rrt_plan(Player::Ptr player, Point initial, Point goal) {
		Point nearestPoint, extended, target, projected;

		NodeTree<Point> *nearestNode;
		NodeTree<Point> *lastAdded;
		NodeTree<Point> rrtTree(initial);

		lastAdded = &rrtTree;

		int iterationCounter = 0;

		// should loop until distance between lastAdded and goal is less than threshold
		while ((lastAdded->data() - goal).len() > THRESHOLD && iterationCounter < ITERATION_LIMIT) {
			target = choose_target(goal);
			nearestNode = nearest(&rrtTree, target);
			nearestPoint = nearestNode->data();

			if (nearestNode->parent() == NULL) {
				projected = nearestNode->data() + player->velocity() * TIMESTEP;
			} else {
				projected = 2 * nearestNode->data() - nearestNode->parent()->data();
			}

			extended = extend(player, projected, nearestPoint, target);

			if (is_empty_state(extended)) {
				if (nearestNode->parent() == NULL) {
					break;
				}
			} else {
				lastAdded = nearestNode->append_data(extended);
			}

			iterationCounter++;
		}

		bool foundPath = distance(lastAdded, goal) < THRESHOLD;

		if (!foundPath) {
			// LOG_WARN("Reached limit, path not found");

			// set the last added as the node closest to the goal if we reach the iteration limit
			// because the last added could be anything and we use it for tracing back the path
			lastAdded = nearest(&rrtTree, goal);
		}

		// the final closest point to the goal is where we will trace backwards from
		const NodeTree<Point> *iterator = lastAdded;

		// stores the final path of points
		std::deque<Point> pathPoints;
		pathPoints.push_front(lastAdded->data());

		while (iterator != &rrtTree) {
			iterator = iterator->parent();
			pathPoints.push_front(iterator->data());

			// if we found a plan then add the path's points to the waypoint cache
			// with random replacement
			if (foundPath) {
				currPlayerWaypoints->points[std::rand() % NUM_WAYPOINTS] = iterator->data();
			}
		}

		// remove the front of the list, this is the starting point
		pathPoints.pop_front();
		return pathPoints;
	}

	Navigator::Ptr RRTPhysicsNavigator::create(World &world) {
		const Navigator::Ptr p(new RRTPhysicsNavigator(world));
		return p;
	}

	RRTPhysicsNavigator::RRTPhysicsNavigator(World &world) : Navigator(world) {
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

