#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_planner.h"
#include "ai/navigator/util.h"
#include "ai/param.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;

namespace AI {
	namespace Nav {
		namespace RRT {
			// fraction of the maximum speed that the robot will try to dribble at
			const double DRIBBLE_SPEED = 1.0;

			DoubleParam offset_angle("Pivot: offset angle (degrees)", "Nav/RRT", 30.0, -1000.0, 1000.0);
			DoubleParam offset_distance("Pivot: offset distance", "Nav/RRT", 0.15, -10.0, 10.0);
			DoubleParam orientation_offset("Pivot: orientation offset (degrees)", "Nav/RRT", 30.0, -1000.0, 1000.0);
			
			DoubleParam chase_angle_range("Chase angle range for behind target (degrees)", "Nav/RRT", 30, 0, 90);
			DoubleParam chase_distance("Buffer behind ball for chase (meters)", "Nav/RRT", 0.25, -1.0, 1.0);
			class PlayerData : public ObjectStore::Element {
				public:
					typedef ::RefPtr<PlayerData> Ptr;
					//bool valid;
					unsigned int added_flags;
			};

			class RRTNavigator : public Navigator {
				public:
					NavigatorFactory &factory() const;
					void pivot(Player::Ptr player);
					std::pair<Point, double> grab_ball(Player::Ptr player);
					std::pair<Point, double> grab_ball_orientation(Player::Ptr player);
					std::pair<Point, double> intercept_ball(Player::Ptr player);
					std::pair<Point, double> intercept_ball_orientation(Player::Ptr player);
					void tick();
					static Navigator::Ptr create(World &world);

				private:
					RRTNavigator(World &world);
					RRTPlanner planner;
			};

			class RRTNavigatorFactory : public NavigatorFactory {
				public:
					RRTNavigatorFactory();
					Navigator::Ptr create_navigator(World &world) const;
			};

			RRTNavigatorFactory factory_instance;

			NavigatorFactory &RRTNavigator::factory() const {
				return factory_instance;
			}

			std::pair<Point, double> RRTNavigator::intercept_ball(Player::Ptr player) {
				// TODO: Jason, implement this.
			}
			
			std::pair<Point, double> RRTNavigator::intercept_ball_orientation(Player::Ptr player) {
				// TODO: Jason, implement this.
			}

			std::pair<Point, double> RRTNavigator::grab_ball_orientation(Player::Ptr player) {
				Point dest_pos;

				double dest_ori = (world.ball().position() - player->position()).orientation();

				if (!AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), player->position(), dest_pos)) {
					return std::make_pair(world.ball().position(), dest_ori);
				}

				Point np = world.ball().position();

				Point dir_ball = (player->destination().first - np).norm();
				Point dir = (np - player->position()).norm();
				double rad = chase_angle_range*M_PI/180.0;
				bool op_ori = dir_ball.dot(dir)<cos(rad);
				if (op_ori) {
					PlayerData::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->added_flags |= AI::Flags::FLAG_AVOID_BALL_TINY;
#warning magic number here
					np += chase_distance*( np - player->destination().first).norm();
				}
				dest_pos += np - world.ball().position();

				return std::make_pair(dest_pos, dest_ori);
			}
			
			std::pair<Point, double> RRTNavigator::grab_ball(Player::Ptr player) {
				double dest_ori = (world.ball().position() - player->position()).orientation();

				Point dest_pos;

				if (!AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), player->position(), dest_pos)) {
					return std::make_pair(world.ball().position(), dest_ori);
				}

				return std::make_pair(dest_pos, dest_ori);
			}

			void RRTNavigator::pivot(Player::Ptr player) {
				Player::Path path;
				Point dest;
				double dest_orientation;

				// try to pivot around the ball to catch it
				Point current_position = player->position();
				double to_ball_orientation = (world.ball().position() - current_position).orientation();
				double orientation_temp = degrees2radians(orientation_offset);

				double angle = degrees2radians(offset_angle);
				
				double difference = angle_mod(to_ball_orientation - player->destination().second);
				
				if (difference > 0) {
					angle = -angle;
					orientation_temp = -orientation_temp;
				}

				Point diff = (world.ball().position() - current_position).rotate(angle);

				dest = world.ball().position() - offset_distance * (diff / diff.len());
				dest_orientation = (world.ball().position() - current_position).orientation() + orientation_temp;

				path.push_back(std::make_pair(std::make_pair(dest, dest_orientation), world.monotonic_time()));
				player->path(path);
			}

			void RRTNavigator::tick() {
				timespec working_time;
				Player::Path path;
				std::vector<Point> path_points;

				for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
					path.clear();
					Player::Ptr player = world.friendly_team().get(i);
					if (!PlayerData::Ptr::cast_dynamic(player->object_store()[typeid(*this)]).is()) {
						player->object_store()[typeid(*this)] = PlayerData::Ptr(new PlayerData);
					}
					PlayerData::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->added_flags=0;
					Point dest;
					double dest_orientation = player->destination().second;
					if (player->type() == AI::Flags::MoveType::CATCH) {
						std::pair<Point, double> grab_ball_dest = grab_ball(player);
						dest = grab_ball_dest.first;
						dest_orientation = grab_ball_dest.second;
					} else if (player->type() == AI::Flags::MoveType::CATCH_PIVOT) {
						std::pair<Point, double> grab_ball_dest = grab_ball_orientation(player);
						dest = grab_ball_dest.first;
						dest_orientation = grab_ball_dest.second;
					} else if (player->type() == AI::Flags::MoveType::INTERCEPT) {
						std::pair<Point, double> grab_ball_dest = intercept_ball(player);
						dest = grab_ball_dest.first;
						dest_orientation = grab_ball_dest.second;
					} else if (player->type() == AI::Flags::MoveType::INTERCEPT_PIVOT) {
						std::pair<Point, double> grab_ball_dest = intercept_ball_orientation(player);
						dest = grab_ball_dest.first;
						dest_orientation = grab_ball_dest.second;
					} else if (player->type() == AI::Flags::MoveType::PIVOT) {
						pivot(player);
						continue;
					} else if (player->type() == AI::Flags::MoveType::RAM_BALL){
						Point cur_position = player->position(), dest_position = player->destination().first;
						timespec ts = get_next_ts(world.monotonic_time(), cur_position, dest_position, player->target_velocity());
						path.push_back(std::make_pair(std::make_pair(dest_position, dest_orientation), ts));
						player->path(path);
						continue;
					} else if (valid_path(player->position(), player->destination().first, world, player)) {
						// if we're not trying to catch the ball and there are no obstacles in our way then go
						// to the exact location, skipping all of the tree creation
						path.push_back(std::make_pair(player->destination(), world.monotonic_time()));
						player->path(path);
						continue;
					} else {
						dest = player->destination().first;
					}
					unsigned int flags = PlayerData::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->added_flags;
					// calculate a path
					path_points.clear();
					path_points = planner.plan(player, dest, flags);

					double dist = 0.0;
					working_time = world.monotonic_time();

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
						if (player->type() == AI::Flags::MoveType::DRIBBLE) {
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

			RRTNavigator::RRTNavigator(AI::Nav::W::World &world) : Navigator(world), planner(world) {
			}

			RRTNavigatorFactory::RRTNavigatorFactory() : NavigatorFactory("RRT Navigator") {
			}

			Navigator::Ptr RRTNavigatorFactory::create_navigator(World &world) const {
				return RRTNavigator::create(world);
			}
		}
	}
}

