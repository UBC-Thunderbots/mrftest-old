#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_planner.h"
#include "ai/navigator/util.h"
#include "ai/param.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
// #include "util/param.h"
#include "ai/hl/stp/param.h"

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
			DoubleParam offset_distance("Pivot: offset distance", "Nav/RRT", 0.15, -10.0, 10.0);
			DegreeParam orientation_offset("Pivot: orientation offset (degrees)", "Nav/RRT", 30.0, -1000.0, 1000.0);

			DegreeParam chase_angle_range("Chase angle range for behind target (degrees)", "Nav/RRT", 30, 0.0, 90.0);
			DoubleParam chase_distance("Buffer behind ball for chase (meters)", "Nav/RRT", 0.25, -1.0, 1.0);
			DoubleParam ball_velocity_threshold("Ball velocity threshold (used to switch between chase and chase+pivot)", "Nav/RRT", 0.5, 0.0, 20.0);

			BoolParam use_new_pivot("New Pivot: enable", "Nav/RRT", false);
			DoubleParam new_pivot_linear_sfactor("New Pivot [PID]: linear", "Nav/RRT", 1.0, 0.01, 50.0);
			DoubleParam new_pivot_angular_sfactor("New Pivot [PID]: angular", "Nav/RRT", 1.0, 0.01, 50.0);
			DoubleParam new_pivot_radius("New Pivot: travel radius", "Nav/RRT", 0.3, 0.01, 0.5);
			BoolParam new_pivot_go_backward("New Pivot: go backward", "Nav/RRT", false);
			RadianParam new_pivot_offset_angle("New Pivot: offset angle (radians)", "Nav/RRT", 0.1, 0, 0.5);
			DoubleParam new_pivot_travel_angle("New Pivot: travel angle, need proper unit, (n*M_PI)", "Nav/RRT", 0.2, -0.5, 0.5);
			DoubleParam new_pivot_hyster_angle("New Pivot: Hysterisis angle, one side, (n*M_PI)", "Nav/RRT", 0.2, 0.01, 0.2);
			DoubleParam new_pivot_thresh_angle("New Pivot: Threshold angle, one side, (n*M_PI)", "Nav/RRT", 0.2, 0.01, 0.2);
			BoolParam use_byronic("Byronic", "Nav/RRT", false);
			BoolParam use_new_chase("New chase: enable", "Nav/RRT", false);

			class PlayerData : public ObjectStore::Element {
				public:
					typedef ::RefPtr<PlayerData> Ptr;
					unsigned int added_flags;
			};

			class RRTNavigator : public Navigator {
				public:
					NavigatorFactory &factory() const;
					void pivot(Player::Ptr player);
					std::pair<Point, Angle> grab_ball(Player::Ptr player);
					std::pair<Point, Angle> grab_ball_orientation(Player::Ptr player);
					void intercept_ball(Player::Ptr player);
					std::pair<Point, Angle> intercept_ball_orientation(Player::Ptr player);

					void tick();
					static Navigator::Ptr create(World &world);
					void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx);

				private:
					RRTNavigator(World &world);
					RRTPlanner planner;
					bool is_ccw;
					std::pair<Point, Angle> grab_ball_orientation(Player::Ptr player, Point target);
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

			void RRTNavigator::intercept_ball(Player::Ptr player) {
				if (!find_best_intersecting_point(world, player)) {
					LOG_INFO("No intersecting point found!");
				}
			}

			std::pair<Point, Angle> RRTNavigator::intercept_ball_orientation(Player::Ptr player) {
				// TODO: Jason, implement this.
			}

			std::pair<Point, Angle> RRTNavigator::grab_ball_orientation(Player::Ptr player, Point target) {
				Point dest_pos;

				Angle dest_ori = (world.ball().position() - player->position()).orientation();

				if (!AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), player->position(), dest_pos)) {
					LOG_DEBUG("grab fail");
					return std::make_pair(world.ball().position(), dest_ori);
				}

				Point np = world.ball().position();

				Point dir_ball = (target - np).norm();
				Point dir = (np - player->position()).norm();
				bool op_ori = dir_ball.dot(dir) < chase_angle_range.get().cos();
				if (op_ori) {
					PlayerData::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->added_flags |= AI::Flags::FLAG_AVOID_BALL_TINY;
					np += chase_distance * (np - player->destination().first).norm();
				}
				dest_pos += np - world.ball().position();

				return std::make_pair(dest_pos, dest_ori);
			}

			std::pair<Point, Angle> RRTNavigator::grab_ball_orientation(Player::Ptr player) {
				if (world.ball().velocity().len() < ball_velocity_threshold) {
					return grab_ball_orientation(player, player->destination().first);
				}
				Point A = world.ball().velocity().norm();
				Point B = (player->destination().first - world.ball().position()).norm();
				if (A.dot(B) > AI::player_receive_threshold.get().cos()) {
					return grab_ball_orientation(player, player->destination().first);
				}

				double ball_component = (A.dot(B) < 0) ? -1.0 : 1.0;
				A = ball_component * A;

				Angle rotate_amnt = (A.cross(B - A) < 0) ? (AI::player_receive_threshold) : (AI::player_receive_threshold);

				Point target_dir = A.rotate(rotate_amnt);
				return grab_ball_orientation(player, world.ball().position() + target_dir);
			}

			std::pair<Point, Angle> RRTNavigator::grab_ball(Player::Ptr player) {
				if (use_new_chase) {
					// check the angle difference between player and ball velocity:
					Angle angle1 = (player->position() - world.ball().position()).orientation();
					Angle angle2 = world.ball().velocity().orientation();

					if (angle1.angle_diff(angle2) < Angle::of_radians(2.4)) {
						Angle dest_ori = (world.ball().position() - player->position()).orientation();
						Point dest_pos;
						if (!AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), player->position(), dest_pos)) {
							return std::make_pair(world.ball().position(), dest_ori);
						}
						return std::make_pair(dest_pos, dest_ori);
					} else {
						Angle dest_ori = (world.ball().position() - player->position()).orientation();
						Point vec = world.ball().velocity().norm();
						if (vec.len() < ball_velocity_threshold) {
							PlayerData::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->added_flags |= AI::Flags::FLAG_AVOID_BALL_TINY;
						}
						return std::make_pair(world.ball().position() + vec, dest_ori);
					}
				} else {
					Angle dest_ori = (world.ball().position() - player->position()).orientation();
					Point dest_pos;
					if (!AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), player->position(), dest_pos)) {
						return std::make_pair(world.ball().position(), dest_ori);
					}
					return std::make_pair(dest_pos, dest_ori);
				}
			}

			void RRTNavigator::pivot(Player::Ptr player) {
				if (!use_new_pivot) {
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

					dest = world.ball().position() - offset_distance * (diff / diff.len());
					dest_orientation = (world.ball().position() - current_position).orientation() + orientation_temp;

					path.push_back(std::make_pair(std::make_pair(dest, dest_orientation), world.monotonic_time()));
					player->path(path);
				} else { // new pivot is byron's pivot and koko's code is designed for turning with the bal
					Player::Path path;
					if (!player->has_ball()) {
						// use the byron pivot to catch to the ball
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

						dest = world.ball().position() - offset_distance * (diff / diff.len());
						dest_orientation = (world.ball().position() - current_position).orientation() + orientation_temp;

						path.push_back(std::make_pair(std::make_pair(dest, dest_orientation), world.monotonic_time()));

						player->path(path);
					} else {
						Angle diff = ((world.ball().position() - player->destination().first).orientation() - (player->orientation() + (is_ccw ? 1 : -1) * new_pivot_offset_angle)).angle_mod();
						// LOG_INFO( diff );
						LOG_INFO("NEWpiovt!");
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
			}


			void RRTNavigator::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				ctx->set_source_rgb(1.0, 1.0, 1.0);
				for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
					Player::Ptr player = world.friendly_team().get(i);
					if (player->type() == AI::Flags::MoveType::INTERCEPT) {
						find_best_intersecting_point(world, player, ctx);
					}
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
					if (!PlayerData::Ptr::cast_dynamic(player->object_store()[typeid(*this)]).is()) {
						player->object_store()[typeid(*this)] = PlayerData::Ptr(new PlayerData);
					}
					PlayerData::Ptr::cast_dynamic(player->object_store()[typeid(*this)])->added_flags = 0;
					Point dest;
					Angle dest_orientation = player->destination().second;
					if (player->type() == AI::Flags::MoveType::CATCH) {
						std::pair<Point, Angle> grab_ball_dest = grab_ball(player);
						dest = grab_ball_dest.first;
						dest_orientation = grab_ball_dest.second;
					} else if (player->type() == AI::Flags::MoveType::CATCH_PIVOT) {
						std::pair<Point, Angle> grab_ball_dest;

						// Check the ball velocity. If it is travelling too fast, use grab_ball instead of grab_ball_orientation.
						// Based on field testing, we determined that grab_ball_orientation is not effective on moving balls.
						if (world.ball().velocity().len() > ball_velocity_threshold && use_byronic) {
							grab_ball_dest = grab_ball(player);
						} else {
							grab_ball_dest = grab_ball_orientation(player);
						}

						dest = grab_ball_dest.first;
						dest_orientation = grab_ball_dest.second;
					} else if (player->type() == AI::Flags::MoveType::INTERCEPT) {
						intercept_ball(player);
						continue;
					} else if (player->type() == AI::Flags::MoveType::INTERCEPT_PIVOT) {
						std::pair<Point, Angle> grab_ball_dest = intercept_ball_orientation(player);
						dest = grab_ball_dest.first;
						dest_orientation = grab_ball_dest.second;
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

