#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_planner.h"
#include "ai/navigator/util.h"
#include "ai/param.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "ai/hl/stp/param.h"
#include <memory>

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

			DegreeParam offset_angle("Pivot: offset angle (degrees)", "Nav/RRT", 30.0, -1000.0, 1000.0);
			DegreeParam orientation_offset("Pivot: orientation offset (degrees)", "Nav/RRT", 30.0, -1000.0, 1000.0);

			BoolParam use_new_pivot("New Pivot: enable", "Nav/RRT", false);
			DoubleParam new_pivot_linear_sfactor("New Pivot [PID]: linear", "Nav/RRT", 1.0, 0.01, 50.0);
			DoubleParam new_pivot_angular_sfactor("New Pivot [PID]: angular", "Nav/RRT", 1.0, 0.01, 50.0);
			DoubleParam new_pivot_radius("New Pivot: travel radius", "Nav/RRT", 0.3, 0.01, 0.5);
			BoolParam new_pivot_go_backward("New Pivot: go backward", "Nav/RRT", false);
			RadianParam new_pivot_offset_angle("New Pivot: offset angle (radians)", "Nav/RRT", 0.1, 0, 0.5);
			DoubleParam new_pivot_travel_angle("New Pivot: travel angle, need proper unit, (n*M_PI)", "Nav/RRT", 0.2, -0.5, 0.5);
			DoubleParam new_pivot_hyster_angle("New Pivot: Hysterisis angle, one side, (n*M_PI)", "Nav/RRT", 0.2, 0.01, 0.2);
			DoubleParam new_pivot_thresh_angle("New Pivot: Threshold angle, one side, (n*M_PI)", "Nav/RRT", 0.2, 0.01, 0.2);

			class PlayerData : public ObjectStore::Element {
				public:
					typedef std::shared_ptr<PlayerData> Ptr;
					unsigned int added_flags;
			};

			class RRTNavigator : public Navigator {
				public:
					NavigatorFactory &factory() const;
					void tick();
					static Navigator::Ptr create(World &world);
					void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx);

				private:
					void pivot(Player::Ptr player);

					RRTNavigator(World &world);
					RRTPlanner planner;
					bool is_ccw;
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

			void RRTNavigator::pivot(Player::Ptr player) {
				double offset_distance = (player->destination().first - world.ball().position()).len();

				if (!use_new_pivot || !player->has_ball()) {
					Player::Path path;
					Point dest;
					Angle dest_orientation;

					// try to pivot around the ball to catch it
					Point current_position = player->position();
					Angle to_ball_orientation = (world.ball().position() - current_position).orientation();
					Angle orientation_temp = orientation_offset;

					Angle angle = offset_angle;

					Angle difference = (to_ball_orientation - player->destination().second).angle_mod();

					if (difference > Angle::ZERO) {
						angle = -angle;
						orientation_temp = -orientation_temp;
					}

					Point diff = (world.ball().position() - current_position).rotate(angle);

					dest = world.ball().position() - offset_distance * diff.norm();
					dest_orientation = (world.ball().position() - current_position).orientation() + orientation_temp;

					path.push_back(std::make_pair(std::make_pair(dest, dest_orientation), world.monotonic_time()));
					player->path(path);
				} else {
					Player::Path path;

					Angle diff = ((world.ball().position() - player->destination().first).orientation() - (player->orientation() + (is_ccw ? 1 : -1) * new_pivot_offset_angle)).angle_mod();
					// LOG_INFO( diff );
					LOG_INFO("NEWpivot!");
					Point zero_pos(new_pivot_radius, 0.0);
					Point polar_pos;
					Point rel_pos;
					Point dest_pos;
					Angle rel_orient;
					Angle dest_orient;

					// decide on ccw or cw
					if (diff > new_pivot_hyster_angle * Angle::HALF) {
						is_ccw = true;
					} else if (diff < -new_pivot_hyster_angle * Angle::HALF) {
						is_ccw = false;
					}

					// decide on how to get there fast
					if (diff.abs() > new_pivot_thresh_angle * Angle::HALF) {
						rel_orient = new_pivot_travel_angle * Angle::HALF * (is_ccw ? 1 : -1);
						rel_orient *= new_pivot_angular_sfactor;
						polar_pos = zero_pos - zero_pos.rotate(rel_orient);
						rel_pos = polar_pos.rotate(player->orientation() + Angle::QUARTER * (is_ccw ? 1 : -1) * (new_pivot_go_backward ? -1 : 1));
						rel_pos *= new_pivot_linear_sfactor;
						dest_pos = player->position() + rel_pos;
						dest_orient = player->orientation() + rel_orient;
					} else {
						// decide on how to be precise
						rel_orient = diff;
						rel_orient *= new_pivot_angular_sfactor;
						polar_pos = zero_pos - zero_pos.rotate(rel_orient);
						rel_pos = polar_pos.rotate(player->orientation() + Angle::QUARTER);
						rel_pos *= new_pivot_linear_sfactor;
						dest_pos = player->position() + rel_pos;
						dest_orient = (world.ball().position() - player->destination().first).orientation();
					}

					path.push_back(std::make_pair(std::make_pair(dest_pos, dest_orient), world.monotonic_time()));
					player->path(path);
				}
			}


			void RRTNavigator::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				ctx->set_source_rgb(1.0, 1.0, 1.0);
				for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
					Player::Ptr player = world.friendly_team().get(i);
					ctx->begin_new_path();
					ctx->set_line_width(1);
					ctx->move_to(world.ball().position().x, world.ball().position().y);
					ctx->line_to(player->destination().first.x, player->destination().first.y);
				}
			}

			void RRTNavigator::tick() {
				timespec working_time;
				Player::Path path;
				std::vector<Point> path_points;

				for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
					path.clear();
					Player::Ptr player = world.friendly_team().get(i);
					if (!std::dynamic_pointer_cast<PlayerData>(player->object_store()[typeid(*this)])) {
						player->object_store()[typeid(*this)] = std::make_shared<PlayerData>();
					}
					std::dynamic_pointer_cast<PlayerData>(player->object_store()[typeid(*this)])->added_flags = 0;
					Point dest;
					Angle dest_orientation = player->destination().second;
					if (player->type() == AI::Flags::MoveType::INTERCEPT) {
						// refer to this function in util.cpp
						intercept_flag_handler(world, player);
						continue;
					} else if (player->type() == AI::Flags::MoveType::PIVOT) {
						pivot(player);
						continue;
					} else if (player->type() == AI::Flags::MoveType::RAM_BALL) {
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
					unsigned int flags = std::dynamic_pointer_cast<PlayerData>(player->object_store()[typeid(*this)])->added_flags;
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

					player->path(path);
				}
			}

			Navigator::Ptr RRTNavigator::create(World &world) {
				Navigator::Ptr p(new RRTNavigator(world));
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

