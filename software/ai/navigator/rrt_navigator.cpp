#include "ai/navigator/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <uicomponents/param.h>
#include "ai/navigator/rrt_base.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;


namespace AI {
	namespace Nav {
		namespace RRT{

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


			class RRTNavigator : public RRTBase {
			public:
				NavigatorFactory &factory() const;
				void tick();
				static Navigator::Ptr create(World &world);

			private:
				RRTNavigator(World &world);
				~RRTNavigator();

				Waypoints::Ptr curr_player_waypoints;
				unsigned int added_flags;

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

			Navigator::Ptr RRTNavigator::create(World &world) {
				const Navigator::Ptr p(new RRTNavigator(world));
				return p;
			}

			RRTNavigator::RRTNavigator(World &world) : RRTBase(world) {
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
	}
}

