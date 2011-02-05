#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/objectstore.h"
#include <glibmm.h>
#include <uicomponents/param.h>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;

namespace {
	// fraction of the maximum speed that the robot will try to dribble at
	const double DRIBBLE_SPEED = 1.0;
	const double THRESHOLD = 0.08;
	const double STEP_DISTANCE = 0.1;
	// probability that we will take a step towards the goal
	const double GOAL_PROB = 0.2;
	const double WAYPOINT_PROB = 0.5;
	const double RAND_PROB = 1.0 - GOAL_PROB - WAYPOINT_PROB;
	DoubleParam ANGLE_DIFF("Pivot the amount of erroneous angle acceptable for pivoting" , 4.0, 0.00, 10.0);
	// number of iterations to go through for each robot until we give up and
	// just return the best partial path we've found
	const int ITERATION_LIMIT = 200;
	const int NUM_WAYPOINTS = 50;

	DoubleParam pivot_point("Pivot how far behind the ball to go for a pivot", 0.08, 0.00, 1.00);
	DoubleParam chase_overshoot("Pivot  amount of ball overshoot for a pivot", 0.08, -0.5, 0.5);

	class Waypoints : public ObjectStore::Element {
		public:
			typedef ::RefPtr<Waypoints> Ptr;

			Point points[NUM_WAYPOINTS];
	};

	class RRTNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			void tick();
			static Navigator::Ptr create(World &world);

		private:
			RRTNavigator(World &world);
			~RRTNavigator();

			Waypoints::Ptr currPlayerWaypoints;
			unsigned int addedFlags;

			double distance(Point nearest, Point goal);
			Point random_point();
			Point choose_target(Point goal);
			NodeTree<Point> *nearest(NodeTree<Point> *tree, Point target);
			Point empty_state();
			Point extend(Player::Ptr player, Point start, Point target);
			bool is_empty_state(Point toCheck);
			std::vector<Point> rrt_plan(Player::Ptr player, Point initial, Point goal);
	};

	class RRTNavigatorFactory : public NavigatorFactory {
		public:
			RRTNavigatorFactory();
			~RRTNavigatorFactory();
			Navigator::Ptr create_navigator(World &world) const;
	};

	RRTNavigatorFactory factory_instance;

	NavigatorFactory &RRTNavigator::factory() const {
		return factory_instance;
	}

	void RRTNavigator::tick() {
		timespec workingTime;
		std::vector<std::pair<std::pair<Point, double>, timespec> > path;
		std::vector<Point> pathPoints;

		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			path.clear();
			Player::Ptr player = world.friendly_team().get(i);
			
			// temp hack move ram ball does not try to avoid obstacles
			// if (player->type() == MOVE_RAM_BALL) {
				// std::pair<Point, timespec> temp;
				// Point p =  player->destination().first;
				// temp = get_ramball_location( p, world, player);
 				// get_ramball_location( player->destination().first, world, player);
				// path.push_back(std::make_pair(std::make_pair(temp.first, player->destination().second), temp.second));
				// player->path(path);
				// continue;
			// }

			// create new waypoints for the player if they have not been created yet
			if (!player->object_store()[typeid(*this)].is()) {
				Waypoints::Ptr newWaypoints(new Waypoints);
				player->object_store()[typeid(*this)] = newWaypoints;
			}

			currPlayerWaypoints = Waypoints::Ptr::cast_dynamic(player->object_store()[typeid(*this)]);

			addedFlags = 0;
			Point dest;
			if (player->type() == MOVE_CATCH) {
				Point diff = world.ball().position() - player->position();
				diff = diff.rotate(-player->orientation());
				double ang = radians2degrees(diff.orientation());

				if (ang < 0) {
					ang = -ang;
				}

				if (ang < ANGLE_DIFF) {
					// if we are facing in the correct direction go straight to the point
					Point offset_dest = (world.ball().position() - player->position()).norm() * chase_overshoot;
					dest = world.ball().position() + offset_dest;

					// move towards the ball slower so we don't hit it too far away
					timespec timeToBall;
					timespec_add(double_to_timespec(0.4), world.monotonic_time(), timeToBall);
					path.push_back(std::make_pair(std::make_pair(dest, player->destination().second), timeToBall));
					player->path(path);
					continue;
				} else {
					// go to a point behind the ball if the robot isn't in the correct place to catch it
					Point pBehindBall(player->MAX_RADIUS + Ball::RADIUS + pivot_point, 0);
					pBehindBall = pBehindBall.rotate(player->destination().second);
					pBehindBall = Point(world.ball().position() - pBehindBall);
					dest = pBehindBall;
					addedFlags = FLAG_AVOID_BALL_TINY;
				}
			} else if (valid_path(player->position(), player->destination().first, world, player)) {
				// if we're not trying to catch the ball and there are no obstacles in our way then go
				// to the exact location, skipping all of the tree creation
				path.push_back(std::make_pair(player->destination(), world.monotonic_time()));
				player->path(path);
				continue;
			} else {
				dest = player->destination().first;
			}

			// calculate a path
			pathPoints.clear();
			pathPoints = rrt_plan(player, player->position(), dest);

			double dist = 0.0;
			workingTime = world.monotonic_time();

			double destOrientation = player->destination().second;
			for (std::size_t j = 0; j < pathPoints.size(); ++j) {
				// the last point will just use whatever the last orientation was
				if (j + 1 != pathPoints.size()) {
					destOrientation = (pathPoints[j + 1] - pathPoints[j]).orientation();
				}

				// get distance between last two points
				if (j == 0) {
					dist = (player->position() - pathPoints[0]).len();
				} else {
					dist = (pathPoints[j] - pathPoints[j - 1]).len();
				}

				// dribble at a different speed
				if (player->type() == MOVE_DRIBBLE) {
					timespec timeToAdd = double_to_timespec(dist / player->MAX_LINEAR_VELOCITY / DRIBBLE_SPEED);
					timespec_add(workingTime, timeToAdd, workingTime);
				}

				path.push_back(std::make_pair(std::make_pair(pathPoints[j], destOrientation), workingTime));
			}

			// just use the current player position as the destination if we are within the
			// threshold already
			if (pathPoints.size() == 0) {
				path.push_back(std::make_pair(std::make_pair(player->position(), destOrientation), workingTime));
			}

			player->path(path);
		}
	}

	double RRTNavigator::distance(Point nearest, Point goal) {
		return (goal - nearest).len();
	}

	Point RRTNavigator::empty_state() {
		return Point(-10000, -10000);
	}

	bool RRTNavigator::is_empty_state(Point toCheck) {
		return toCheck.x == empty_state().x && toCheck.y && empty_state().y;
	}

	// generate a random point from the field
	Point RRTNavigator::random_point() {
		double randomX = ((std::rand() % static_cast<int>(world.field().length() * 100)) - (world.field().length() * 50)) / 100;
		double randomY = ((std::rand() % static_cast<int>(world.field().width() * 100)) - (world.field().width() * 50)) / 100;

		return Point(randomX, randomY);
	}

	// choose a target to extend toward, the goal, a waypoint or a random point
	Point RRTNavigator::choose_target(Point goal) {
		double p = std::rand() / static_cast<double>(RAND_MAX);
		int i = std::rand() % NUM_WAYPOINTS;

		if (p > 0 && p <= WAYPOINT_PROB) {
			return currPlayerWaypoints->points[i];
		} else if (p > WAYPOINT_PROB && p < (WAYPOINT_PROB + RAND_PROB)) {
			return random_point();
		} else {
			return goal;
		}
	}

	// finds the point in the tree that is nearest to the target point
	NodeTree<Point> *RRTNavigator::nearest(NodeTree<Point> *rrtTree, Point target) {
		NodeTree<Point> *nearest = rrtTree;
		NodeTree<Point> *currNode;

		std::vector<NodeTree<Point> *> nodeQueue;
		nodeQueue.push_back(rrtTree);

		// iterate through all the nodes in the tree, finding which is closest to the target
		while (nodeQueue.size() > 0) {
			currNode = nodeQueue.back();
			nodeQueue.pop_back();

			if (distance(currNode->data(), target) < distance(nearest->data(), target)) {
				nearest = currNode;
			}

			for (unsigned int i = 0; i < currNode->child_count(); ++i) {
				nodeQueue.push_back(currNode->nth_child(i));
			}
		}

		return nearest;
	}

	// extend by STEP_DISTANCE towards the target from the start
	Point RRTNavigator::extend(Player::Ptr player, Point start, Point target) {
		Point extendPoint = start + ((target - start).norm() * STEP_DISTANCE);

		// check if the point is invalid (collision, out of bounds, etc...)
		// if it is then return EmptyState()
		if (!valid_path(start, extendPoint, world, player, addedFlags)) {
			return empty_state();
		}

		return extendPoint;
	}

	std::vector<Point> RRTNavigator::rrt_plan(Player::Ptr player, Point initial, Point goal) {
		Point nearestPoint, extended, target;

		NodeTree<Point> *nearestNode;
		NodeTree<Point> *lastAdded;
		NodeTree<Point> rrtTree(initial);

		lastAdded = &rrtTree;

		int iterationCounter = 0;

		// should loop until distance between lastAdded and goal is less than threshold
		while (distance(lastAdded->data(), goal) > THRESHOLD && iterationCounter < ITERATION_LIMIT) {
			target = choose_target(goal);
			nearestNode = nearest(&rrtTree, target);
			nearestPoint = nearestNode->data();
			extended = extend(player, nearestPoint, target);

			if (!is_empty_state(extended)) {
				lastAdded = nearestNode->append_data(extended);
			}

			iterationCounter++;
		}

		bool foundPath = (iterationCounter != ITERATION_LIMIT);

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

		// path post processing, try to go in a straight line until we hit an obstacle
		std::size_t subPathIndex = 0;
		std::vector<Point> finalPoints;

		for (std::size_t i = 0; i < pathPoints.size(); ++i) {
			if (!valid_path(pathPoints[subPathIndex], pathPoints[i], world, player, addedFlags)) {
				subPathIndex = i - 1;
				finalPoints.push_back(pathPoints[i - 1]);
			} else if (i == pathPoints.size() - 1) {
				finalPoints.push_back(pathPoints[i]);
			}
		}

		return finalPoints;
	}

	Navigator::Ptr RRTNavigator::create(World &world) {
		const Navigator::Ptr p(new RRTNavigator(world));
		return p;
	}

	RRTNavigator::RRTNavigator(World &world) : Navigator(world) {
	}

	RRTNavigator::~RRTNavigator() {
	}

	RRTNavigatorFactory::RRTNavigatorFactory() : NavigatorFactory("RRT Navigator") {
	}

	RRTNavigatorFactory::~RRTNavigatorFactory() {
	}

	Navigator::Ptr RRTNavigatorFactory::create_navigator(World &world) const {
		return RRTNavigator::create(world);
	}
}

