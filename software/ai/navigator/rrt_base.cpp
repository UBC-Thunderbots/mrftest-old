#include "ai/navigator/util.h"
#include "geom/angle.h"
#include "util/dprint.h"

#include <uicomponents/param.h>
#include "ai/navigator/rrt_base.h"

using AI::Nav::Navigator;
using namespace AI::Nav::RRT;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;

#include <iostream>
namespace {
	// fraction of the maximum speed that the robot will try to dribble at
	const double DRIBBLE_SPEED = 1.0;
	const double THRESHOLD = 0.08;
	const double STEP_DISTANCE = 0.1;
	// probability that we will take a step towards the goal
	const double GOAL_PROB = 0.2;
	const double WAYPOINT_PROB = 0.5;
	const double RAND_PROB = 1.0 - GOAL_PROB - WAYPOINT_PROB;
	DoubleParam ANGLE_DIFF("Pivot the amount of erroneous angle acceptable for pivoting", 4.0, 0.00, 10.0);
	// number of iterations to go through for each robot until we give up and
	// just return the best partial path we've found
	const int ITERATION_LIMIT = 200;


	double distance(Point nearest, Point goal) {
		return (goal - nearest).len();
	}

	Point empty_state() {
		return Point(-10000, -10000);
	}

	bool is_empty_state(Point toCheck) {
		return toCheck.x == empty_state().x && toCheck.y && empty_state().y;
	}

}


	// generate a random point from the field
	Point AI::Nav::RRT::RRTBase::random_point() {
		double randomX = ((std::rand() % static_cast<int>(world.field().length() * 100)) - (world.field().length() * 50)) / 100;
		double randomY = ((std::rand() % static_cast<int>(world.field().width() * 100)) - (world.field().width() * 50)) / 100;

		return Point(randomX, randomY);
	}

	// choose a target to extend toward, the goal, a waypoint or a random point
Point RRTBase::choose_target(Point goal, Player::Ptr player) {
		double p = std::rand() / static_cast<double>(RAND_MAX);
		size_t i = std::rand() % Waypoints::NUM_WAYPOINTS;

		if (p > 0 && p <= WAYPOINT_PROB) {
			return Waypoints::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->points[i];
		} else if (p > WAYPOINT_PROB && p < (WAYPOINT_PROB + RAND_PROB)) {
			return random_point();
		} else {
			return goal;
		}
	}

	// finds the point in the tree that is nearest to the target point
Glib::NodeTree<Point> *RRTBase::nearest(Glib::NodeTree<Point> *rrtTree, Point target) {
		Glib::NodeTree<Point> *nearest = rrtTree;
		Glib::NodeTree<Point> *currNode;

		std::vector<Glib::NodeTree<Point> *> nodeQueue;
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
	Point RRTBase::extend(Player::Ptr player, Point start, Point target) {
		Point extendPoint = start + ((target - start).norm() * STEP_DISTANCE);

		if (!valid_path(start, extendPoint, world, player, Waypoints::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->addedFlags)) {
			return empty_state();
		}

		return extendPoint;
	}

	std::vector<Point> RRTBase::rrt_plan(Player::Ptr player, Point initial, Point goal) {

		if( !Waypoints::Ptr::cast_dynamic(player->object_store()[typeid(*this)]).is()){
			player->object_store()[typeid(*this)] = Waypoints::Ptr(new Waypoints);
		}

		Point nearestPoint, extended, target;
		Glib::NodeTree<Point> *nearestNode;
		Glib::NodeTree<Point> *lastAdded;
		Glib::NodeTree<Point> rrtTree(initial);

		lastAdded = &rrtTree;

		int iterationCounter = 0;

		// should loop until distance between lastAdded and goal is less than threshold
		while (distance(lastAdded->data(), goal) > THRESHOLD && iterationCounter < ITERATION_LIMIT) {
			target = choose_target(goal, player);
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
		const Glib::NodeTree<Point> *iterator = lastAdded;

		// stores the final path of points
		std::deque<Point> pathPoints;
		pathPoints.push_front(lastAdded->data());

		while (iterator != &rrtTree) {
			iterator = iterator->parent();
			pathPoints.push_front(iterator->data());

			// if we found a plan then add the path's points to the waypoint cache
			// with random replacement
			if (foundPath) {
				Waypoints::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->points[std::rand() % Waypoints::NUM_WAYPOINTS] = iterator->data();
			}
		}

		// remove the front of the list, this is the starting point
		pathPoints.pop_front();

		// path post processing, try to go in a straight line until we hit an obstacle
		std::size_t subPathIndex = 0;
		std::vector<Point> finalPoints;

		for (std::size_t i = 0; i < pathPoints.size(); ++i) {
			if (!valid_path(pathPoints[subPathIndex], pathPoints[i], world, player,  Waypoints::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->addedFlags)) {
				subPathIndex = i - 1;
				finalPoints.push_back(pathPoints[i - 1]);
			} else if (i == pathPoints.size() - 1) {
				finalPoints.push_back(pathPoints[i]);
			}
		}

		return finalPoints;
	}

	RRTBase::RRTBase(World &world) : Navigator(world) {
	}

	RRTBase::~RRTBase() {
	}




