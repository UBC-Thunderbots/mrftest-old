#include "ai/hl/stp/evaluation/rrt_planner.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include <memory>
#include <algorithm>

using namespace AI::HL::STP;
using namespace AI::HL::STP::Evaluation::Plan;
using namespace AI::Flags;

namespace {
	IntParam iteration_limit(u8"Number of iterations to go through before we give best partial path", u8"AI/Nav/RRT", 200, 10, 2000);
	DoubleParam threshold(u8"Distance to destination when we stop looking for a path (m)", u8"AI/Nav/RRT", 0.08, 0, 1.0);
	DoubleParam step_distance(u8"Distance to extend the tree on each step (m)", u8"AI/Nav/RRT", 0.1, 0, 1.0);

	// probability that we will take a step towards the goal
	constexpr double GOAL_PROB = 0.2;
	constexpr double WAYPOINT_PROB = 0.5;
	constexpr double RAND_PROB = 1.0 - GOAL_PROB - WAYPOINT_PROB;
	constexpr bool POST_PROCESS = true;
	constexpr double EPS = 1e-9;

	bool is_empty_state(Point to_check) {
		return (to_check - Evaluation::RRT::empty_state()).lensq() < EPS;
	}

}

//constexpr std::size_t Waypoints::NUM_WAYPOINTS;

double Evaluation::RRT::distance(Glib::NodeTree<Point> *node, Point goal) {
	return (node->data() - goal).len();
}

// generate a random point from the field
Point Evaluation::RRT::random_point(World world) {
    // divides lenght and width into grid of 100 by 100
	double random_x = ((std::rand() % static_cast<int>(world.field().length() * 100)) - (world.field().length() * 50)) / 100;
	double random_y = ((std::rand() % static_cast<int>(world.field().width() * 100)) - (world.field().width() * 50)) / 100;

	return Point(random_x, random_y);
}

// choose a target to extend toward, the goal, a waypoint or a random point
Point Evaluation::RRT::choose_target(World world, Point goal, Player player, std::vector<Point> way_points) {
	double p = std::rand() / static_cast<double>(RAND_MAX);
    
	if (p > 0 && p <= WAYPOINT_PROB && way_points.size() > 0) {
        std::size_t i = static_cast<std::size_t>(std::rand()) % way_points.size();
		return way_points[i];
	} else if (p > WAYPOINT_PROB && p < (WAYPOINT_PROB + RAND_PROB)) {
		return random_point(world);
	} else {
		return goal;
	}
}

// finds the point in the tree that is nearest to the target point
Glib::NodeTree<Point> * Evaluation::RRT::nearest(Glib::NodeTree<Point> *rrt_tree, Point target) {
	Glib::NodeTree<Point> *nearest = rrt_tree;
	Glib::NodeTree<Point> *curr_node;

	std::vector<Glib::NodeTree<Point> *> node_queue;
	node_queue.push_back(rrt_tree);

	// iterate through all the nodes in the tree, finding which is closest to the target
	while (!node_queue.empty()) {
		curr_node = node_queue.back();
		node_queue.pop_back();

		if (distance(curr_node, target) < distance(nearest, target)) {
			nearest = curr_node;
		}

		for (unsigned int i = 0; i < curr_node->child_count(); ++i) {
			node_queue.push_back(curr_node->nth_child(static_cast<int>(i)));
		}
	}

	return nearest;
}

// extend by STEP_DISTANCE towards the target from the start
Point Evaluation::RRT::extend(World world, Player player, Glib::NodeTree<Point> *start, Point target, MoveFlags added_flags) {
	Point extend_point = start->data() + ((target - start->data()).norm() * step_distance);

	if (!valid_path(start->data(), extend_point, world, player, added_flags)) {
		return empty_state();
	}

	return extend_point;
}


std::vector<Point> Evaluation::RRT::rrt_plan(World world, Player player, Point goal, std::vector<Point> way_points, bool post_process, MoveFlags added_flags) {
	Point initial = player.position();
	Point nearest_point, extended, target;
	Glib::NodeTree<Point> *nearest_node;
	Glib::NodeTree<Point> *last_added;
	Glib::NodeTree<Point> rrt_tree(initial);

	last_added = &rrt_tree;

	int iteration_counter = 0;

	// should loop until distance between the last node added and goal is less than threshold
	while (distance(last_added, goal) > threshold && iteration_counter < iteration_limit) {
		target = choose_target(world, goal, player, way_points);
		nearest_node = nearest(&rrt_tree, target);
		extended = extend(world, player, nearest_node, target, added_flags);

		if (!is_empty_state(extended)) {
			last_added = nearest_node->append_data(extended);
		}

		iteration_counter++;
	}

	bool found_path = (iteration_counter != iteration_limit);

	if (!found_path) {
		// LOG_WARN(u8"Reached limit, path not found");

		// set the last added as the node closest to the goal if we reach the iteration limit
		// because the last added could be anything and we use it for tracing back the path
		last_added = nearest(&rrt_tree, goal);
	}

	// the final closest point to the goal is where we will trace backwards from
	const Glib::NodeTree<Point> *iterator = last_added;

	// stores the final path of points
	std::deque<Point> path_points;
	path_points.push_front(last_added->data());

	while (iterator != &rrt_tree) {
		// keep adding the node's parents until we get to the root
		iterator = iterator->parent();
		path_points.push_front(iterator->data());

		// if we found a plan then add the path's points to the waypoint cache
		// with random replacement
		if (found_path) {
			//std::dynamic_pointer_cast<Waypoints>(player.way_points[static_cast<std::size_t>(std::rand()) % player.NUM_WAYPOINTS] = iterator->data();
		}
	}

	// remove the front of the list, this is the starting point
	path_points.pop_front();

	if (!post_process) {
		std::vector<Point> ans(path_points.begin(), path_points.end());
		return ans;
	}

	// path post processing, try to go in a straight line until we hit an obstacle
	std::size_t sub_path_index = 0;
	std::vector<Point> final_points;

	for (std::size_t i = 0; i < path_points.size(); ++i) {
		if (!valid_path(path_points[sub_path_index], path_points[i], world, player, added_flags)) {
			sub_path_index = i - 1;
			final_points.push_back(path_points[i - 1]);
		} else if (i == path_points.size() - 1) {
			final_points.push_back(path_points[i]);
		}
	}

	// just use the current player position as the destination if we are within the threshold already
	if (final_points.empty()) {
		final_points.push_back(player.position());
	} else if (valid_path(final_points[final_points.size() - 1], goal, world, player)) {
		// go exactly to the goal if we're able
		final_points.push_back(goal);
	}

	return final_points;
}

