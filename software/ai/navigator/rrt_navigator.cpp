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
	DoubleParam ANGLE_DIFF("Pivot the amount of erroneous angle acceptable for pivoting", 4.0, 0.00, 10.0);
	// number of iterations to go through for each robot until we give up and
	// just return the best partial path we've found
	const int ITERATION_LIMIT = 200;
	const int NUM_WAYPOINTS = 50;

	DoubleParam offset_angle("RRT Pivot: offset angle (degrees)", 30.0, -1000.0, 1000.0);
	DoubleParam offset_distance("RRT Pivot: offset distance", 0.15, -10.0, 10.0);
	DoubleParam orientation_offset("RRT Pivot: orientation offset (degrees)", 30.0, -1000.0, 1000.0);

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

			Waypoints::Ptr curr_player_waypoints;
			unsigned int added_flags;

			double distance(Point nearest, Point goal);
			Point random_point();
			Point choose_target(Point goal);
			NodeTree<Point> *nearest(NodeTree<Point> *tree, Point target);
			Point empty_state();
			Point extend(Player::Ptr player, Point start, Point target);
			bool is_empty_state(Point to_check);
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
		timespec working_time;
		Player::Path path;
		std::vector<Point> path_points;

		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			path.clear();
			Player::Ptr player = world.friendly_team().get(i);

			// temp hack move ram ball does not try to avoid obstacles
			// if (player->type() == MOVE_RAM_BALL) {
				// Player::PathPoint temp;
				// Point p =  player->destination().first;
				// temp = get_ramball_location( p, world, player);
				// get_ramball_location( player->destination().first, world, player);
				// path.push_back(std::make_pair(std::make_pair(temp.first, player->destination().second), temp.second));
				// player->path(path);
				// continue;
			// }

			// create new waypoints for the player if they have not been created yet
			if (!player->object_store()[typeid(*this)].is()) {
				Waypoints::Ptr new_waypoints(new Waypoints);
				player->object_store()[typeid(*this)] = new_waypoints;
			}

			curr_player_waypoints = Waypoints::Ptr::cast_dynamic(player->object_store()[typeid(*this)]);

			added_flags = 0;
			Point dest;
			double dest_orientation;
			if (player->type() == MOVE_CATCH) {

				// try to pivot around the ball to catch it
				Point current_position = player->position();
				double to_ball_orientation = (world.ball().position() - current_position).orientation();
				double orientation_temp = degrees2radians(orientation_offset);

				double angle = offset_angle;
				if (angle_mod(to_ball_orientation - player->destination().second) > 0) {
					angle = -angle;
					orientation_temp = -orientation_temp;
				}

				angle = degrees2radians(angle);
				if (fabs(angle_diff(to_ball_orientation, player->destination().second)) < fabs(angle)) {
					if (fabs(angle_diff(to_ball_orientation, player->destination().second)) < fabs(angle)/4) {
						timespec time_to_ball;
						timespec_add(double_to_timespec(0.0), world.monotonic_time(), time_to_ball);
						path.push_back(std::make_pair(std::make_pair(world.ball().position(), player->destination().second), time_to_ball));
						player->path(path);
						continue;
					}
					orientation_temp = 0;
					if (angle < 0) {
						angle = -fabs(angle_diff(to_ball_orientation, player->destination().second));
					} else {
						angle = fabs(angle_diff(to_ball_orientation, player->destination().second));
					}
				}
				Point diff = (world.ball().position() - current_position).rotate(angle);

				dest = world.ball().position() - offset_distance * (diff / diff.len());
				if (dest.len() > 0.5) orientation_temp = 0;
				dest_orientation = (world.ball().position() - current_position).orientation() + orientation_temp;

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
			path_points.clear();
			path_points = rrt_plan(player, player->position(), dest);

			double dist = 0.0;
			working_time = world.monotonic_time();

			dest_orientation = player->destination().second;
			for (std::size_t j = 0; j < path_points.size(); ++j) {
				// the last point will just use whatever the last orientation was
				if (j + 1 != path_points.size()) {
					dest_orientation = (path_points[j + 1] - path_points[j]).orientation();
				}

				// get distance between last two points
				if (j == 0) {
					dist = (player->position() - path_points[0]).len();
				} else {
					dist = (path_points[j] - path_points[j - 1]).len();
				}

				// dribble at a different speed
				if (player->type() == MOVE_DRIBBLE) {
					timespec time_to_add = double_to_timespec(dist / player->MAX_LINEAR_VELOCITY / DRIBBLE_SPEED);
					timespec_add(working_time, time_to_add, working_time);
				}

				path.push_back(std::make_pair(std::make_pair(path_points[j], dest_orientation), working_time));
			}

			// just use the current player position as the destination if we are within the
			// threshold already
			if (path_points.size() == 0) {
				path.push_back(std::make_pair(std::make_pair(player->position(), dest_orientation), working_time));
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

	bool RRTNavigator::is_empty_state(Point to_check) {
		return to_check.x == empty_state().x && to_check.y && empty_state().y;
	}

	// generate a random point from the field
	Point RRTNavigator::random_point() {
		double random_x = ((std::rand() % static_cast<int>(world.field().length() * 100)) - (world.field().length() * 50)) / 100;
		double random_y = ((std::rand() % static_cast<int>(world.field().width() * 100)) - (world.field().width() * 50)) / 100;

		return Point(random_x, random_y);
	}

	// choose a target to extend toward, the goal, a waypoint or a random point
	Point RRTNavigator::choose_target(Point goal) {
		double p = std::rand() / static_cast<double>(RAND_MAX);
		int i = std::rand() % NUM_WAYPOINTS;

		if (p > 0 && p <= WAYPOINT_PROB) {
			return curr_player_waypoints->points[i];
		} else if (p > WAYPOINT_PROB && p < (WAYPOINT_PROB + RAND_PROB)) {
			return random_point();
		} else {
			return goal;
		}
	}

	// finds the point in the tree that is nearest to the target point
	NodeTree<Point> *RRTNavigator::nearest(NodeTree<Point> *rrt_tree, Point target) {
		NodeTree<Point> *nearest = rrt_tree;
		NodeTree<Point> *curr_node;

		std::vector<NodeTree<Point> *> node_queue;
		node_queue.push_back(rrt_tree);

		// iterate through all the nodes in the tree, finding which is closest to the target
		while (node_queue.size() > 0) {
			curr_node = node_queue.back();
			node_queue.pop_back();

			if (distance(curr_node->data(), target) < distance(nearest->data(), target)) {
				nearest = curr_node;
			}

			for (unsigned int i = 0; i < curr_node->child_count(); ++i) {
				node_queue.push_back(curr_node->nth_child(i));
			}
		}

		return nearest;
	}

	// extend by STEP_DISTANCE towards the target from the start
	Point RRTNavigator::extend(Player::Ptr player, Point start, Point target) {
		Point extend_point = start + ((target - start).norm() * STEP_DISTANCE);

		// check if the point is invalid (collision, out of bounds, etc...)
		// if it is then return EmptyState()
		if (!valid_path(start, extend_point, world, player, added_flags)) {
			return empty_state();
		}

		return extend_point;
	}

	std::vector<Point> RRTNavigator::rrt_plan(Player::Ptr player, Point initial, Point goal) {
		Point nearest_point, extended, target;

		NodeTree<Point> *nearest_node;
		NodeTree<Point> *last_added;
		NodeTree<Point> rrt_tree(initial);

		last_added = &rrt_tree;

		int iteration_counter = 0;

		// should loop until distance between lastAdded and goal is less than threshold
		while (distance(last_added->data(), goal) > THRESHOLD && iteration_counter < ITERATION_LIMIT) {
			target = choose_target(goal);
			nearest_node = nearest(&rrt_tree, target);
			nearest_point = nearest_node->data();
			extended = extend(player, nearest_point, target);

			if (!is_empty_state(extended)) {
				last_added = nearest_node->append_data(extended);
			}

			iteration_counter++;
		}

		bool found_path = (iteration_counter != ITERATION_LIMIT);

		if (!found_path) {
			// LOG_WARN("Reached limit, path not found");

			// set the last added as the node closest to the goal if we reach the iteration limit
			// because the last added could be anything and we use it for tracing back the path
			last_added = nearest(&rrt_tree, goal);
		}

		// the final closest point to the goal is where we will trace backwards from
		const NodeTree<Point> *iterator = last_added;

		// stores the final path of points
		std::deque<Point> path_points;
		path_points.push_front(last_added->data());

		while (iterator != &rrt_tree) {
			iterator = iterator->parent();
			path_points.push_front(iterator->data());

			// if we found a plan then add the path's points to the waypoint cache
			// with random replacement
			if (found_path) {
				curr_player_waypoints->points[std::rand() % NUM_WAYPOINTS] = iterator->data();
			}
		}

		// remove the front of the list, this is the starting point
		path_points.pop_front();

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

		return final_points;
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

