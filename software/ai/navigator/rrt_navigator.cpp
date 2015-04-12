#include "ai/navigator/rrt_navigator.h"
#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_planner.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "ai/hl/stp/param.h"
#include "ai/navigator/util.h"

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
			const double DRIBBLE_SPEED = 0.4;

			DegreeParam offset_angle(u8"Pivot offset angle (deg)", u8"AI/Navigator/RRT", 30.0, -1000.0, 1000.0);
			DegreeParam orientation_offset(u8"Pivot orientation offset (deg)", u8"AI/Navigator/RRT", 30.0, -1000.0, 1000.0);

			BoolParam use_new_pivot(u8"New pivot enable", u8"AI/Navigator/RRT", false);
			DoubleParam new_pivot_linear_sfactor(u8"New pivot [PID] linear", u8"AI/Navigator/RRT", 1.0, 0.01, 50.0);
			DoubleParam new_pivot_angular_sfactor(u8"New pivot [PID] angular", u8"AI/Navigator/RRT", 1.0, 0.01, 50.0);
			DoubleParam new_pivot_radius(u8"New pivot travel radius", u8"AI/Navigator/RRT", 0.3, 0.01, 0.5);
			BoolParam new_pivot_go_backward(u8"New pivot go backward", u8"AI/Navigator/RRT", false);
			RadianParam new_pivot_offset_angle(u8"New pivot offset angle (rad)", u8"AI/Navigator/RRT", 0.1, 0, 0.5);
			DoubleParam new_pivot_travel_angle(u8"New pivot travel angle (x*pi rad)", u8"AI/Navigator/RRT", 0.2, -0.5, 0.5);
			DoubleParam new_pivot_hyster_angle(u8"New pivot hysteresis angle, (x*pi rad)", u8"AI/Navigator/RRT", 0.2, 0.01, 0.2);
			DoubleParam new_pivot_thresh_angle(u8"New pivot threshold angle, (x*pi rad)", u8"AI/Navigator/RRT", 0.2, 0.01, 0.2);
			DoubleParam careful_max_speed(u8"Careful max speed", u8"AI/Navigator/RRT", 0.75, 0.1, 3.0);

			IntParam jon_hysteris_hack(u8"Jon Hysteris Hack", u8"AI/Navigator/RRT", 2, 1, 10);


			class RRTNavigator final : public Navigator {
				public:
					explicit RRTNavigator(World world);
					void tick() override;
					void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) override;
					NavigatorFactory &factory() const override;

				private:
					void pivot(Player player);

					RRTPlanner planner;
					bool is_ccw;
			};
		}
	}
}

using AI::Nav::RRT::RRTNavigator;

RRTNavigator::RRTNavigator(AI::Nav::W::World world) : Navigator(world), planner(world) {
}

void RRTNavigator::pivot(Player player) {
	double offset_distance = (player.destination().first - world.ball().position()).len();

	PlayerData::Ptr player_data = std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)]);

	if (!use_new_pivot || !player.has_ball()) {
		Player::Path path;
		Point dest;
		Angle dest_orientation;

		// try to pivot around the ball to catch it
		Point current_position = player.position();
		Angle to_ball_orientation = (world.ball().position() - current_position).orientation();
		Angle orientation_temp = orientation_offset;

		Angle angle = offset_angle;

		Angle difference = (to_ball_orientation - player.destination().second).angle_mod();

		if (difference > Angle::zero()) {
			angle = -angle;
			orientation_temp = -orientation_temp;
		}

		Point diff = (world.ball().position() - current_position).rotate(angle);

		dest = world.ball().position() - offset_distance * diff.norm();
		dest_orientation = (world.ball().position() - current_position).orientation() + orientation_temp;

		if (player_data->prev_move_type == player.type() && player_data->prev_move_prio == player.prio() && player_data->prev_avoid_distance == player.avoid_distance()) {
			dest = (player_data->previous_dest + dest ) / jon_hysteris_hack;
			player_data->previous_dest = dest;
			dest_orientation = (player_data->previous_orient + dest_orientation) / jon_hysteris_hack;
			player_data->previous_orient = dest_orientation;
		}

		path.push_back(std::make_pair(std::make_pair(dest, dest_orientation), world.monotonic_time()));
		player.path(path);
	} else {
		Player::Path path;

		Angle diff = ((world.ball().position() - player.destination().first).orientation() - (player.orientation() + (is_ccw ? 1 : -1) * new_pivot_offset_angle)).angle_mod();
		// LOG_INFO( diff );
		LOG_INFO(u8"NEWpivot!");
		Point zero_pos(new_pivot_radius, 0.0);
		Point polar_pos;
		Point rel_pos;
		Point dest_pos;
		Angle rel_orient;
		Angle dest_orient;

		// decide on ccw or cw
		if (diff > new_pivot_hyster_angle * Angle::half()) {
			is_ccw = true;
		} else if (diff < -new_pivot_hyster_angle * Angle::half()) {
			is_ccw = false;
		}

		// decide on how to get there fast
		if (diff.abs() > new_pivot_thresh_angle * Angle::half()) {
			rel_orient = new_pivot_travel_angle * Angle::half() * (is_ccw ? 1 : -1);
			rel_orient *= new_pivot_angular_sfactor;
			polar_pos = zero_pos - zero_pos.rotate(rel_orient);
			rel_pos = polar_pos.rotate(player.orientation() + Angle::quarter() * (is_ccw ? 1 : -1) * (new_pivot_go_backward ? -1 : 1));
			rel_pos *= new_pivot_linear_sfactor;
			dest_pos = player.position() + rel_pos;
			dest_orient = player.orientation() + rel_orient;
		} else {
			// decide on how to be precise
			rel_orient = diff;
			rel_orient *= new_pivot_angular_sfactor;
			polar_pos = zero_pos - zero_pos.rotate(rel_orient);
			rel_pos = polar_pos.rotate(player.orientation() + Angle::quarter());
			rel_pos *= new_pivot_linear_sfactor;
			dest_pos = player.position() + rel_pos;
			dest_orient = (world.ball().position() - player.destination().first).orientation();
		}

		if (player_data->prev_move_type == player.type() && player_data->prev_move_prio == player.prio() && player_data->prev_avoid_distance == player.avoid_distance()) {
			dest_pos = (player_data->previous_dest + dest_pos ) / jon_hysteris_hack;
			player_data->previous_dest = dest_pos;
			dest_orient = (player_data->previous_orient + dest_orient) / jon_hysteris_hack;
			player_data->previous_orient = dest_orient;
		}

		path.push_back(std::make_pair(std::make_pair(dest_pos, dest_orient), world.monotonic_time()));
		player.path(path);
	}
}

void RRTNavigator::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
	ctx->set_source_rgb(1.0, 1.0, 1.0);
	for (const Player player : world.friendly_team()) {
		ctx->begin_new_path();
		ctx->set_line_width(1);
		ctx->move_to(world.ball().position().x, world.ball().position().y);
		ctx->line_to(player.destination().first.x, player.destination().first.y);
	}
}

void RRTNavigator::tick() {
	for (Player player : world.friendly_team()) {
		if (!std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)])) {
			player.object_store()[typeid(*this)] = std::make_shared<PlayerData>();
		}
		PlayerData::Ptr player_data = std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)]);
		player_data->added_flags = 0;
		Point dest;
		Angle dest_orientation = player.destination().second;
		Player::Path path;

		if (player.type() == AI::Flags::MoveType::INTERCEPT) {
			// refer to this function in util.cpp
			intercept_flag_handler(world, player, player_data);
			continue;
		} else if (player.type() == AI::Flags::MoveType::PIVOT) {
			pivot(player);
			continue;
		} else if (player.type() == AI::Flags::MoveType::RAM_BALL) {
			Point cur_position = player.position(), dest_position = player.destination().first;
			AI::Timestamp ts = get_next_ts(world.monotonic_time(), cur_position, dest_position, player.target_velocity());

			if (player_data->prev_move_type == player.type() && player_data->prev_move_prio == player.prio() && player_data->prev_avoid_distance == player.avoid_distance()) {
				dest_position = (player_data->previous_dest + dest_position ) / jon_hysteris_hack;
				player_data->previous_dest = cur_position;
				dest_orientation = (player_data->previous_orient + dest_orientation) / jon_hysteris_hack;
				player_data->previous_orient = dest_orientation;

			}

				path.push_back(std::make_pair(std::make_pair(dest_position, dest_orientation), ts));
			player.path(path);


			continue;
		} else if (valid_path(player.position(), player.destination().first, world, player)) {
			// if we're not trying to catch the ball and there are no obstacles in our way then go
			// to the exact location, skipping all of the tree creation
			AI::Timestamp ts = world.monotonic_time();
			if (player.flags() & AI::Flags::FLAG_CAREFUL) {
				Point delta = player.destination().first - player.position();
				double distance = delta.len();
				ts += std::chrono::duration_cast<AI::Timediff>(std::chrono::duration<double>(distance / careful_max_speed));
			}
			path.push_back(std::make_pair(player.destination(), ts));
			player.path(path);
			continue;
		} else {

			//this saves next destination passed from the HL to add hystersis in the average filter.
			dest = player.destination().first;

			if (player_data->prev_move_type == player.type() && player_data->prev_move_prio == player.prio() && player_data->prev_avoid_distance == player.avoid_distance()) {
				dest = (player_data->previous_dest + dest ) / jon_hysteris_hack;
				player_data->previous_dest = dest;
				dest_orientation = (player_data->previous_orient + dest_orientation) / jon_hysteris_hack;
				player_data->previous_orient = dest_orientation;
			}

			unsigned int flags = std::dynamic_pointer_cast<PlayerData>(player.object_store()[typeid(*this)])->added_flags;
			// calculate a path
			std::vector<Point> path_points = planner.plan(player, dest, flags);

			double dist = 0.0;
			AI::Timestamp working_time = world.monotonic_time();

			for (std::size_t j = 0; j < path_points.size(); ++j) {
				// the last point will just use whatever the last orientation was
				if (j + 1 != path_points.size()) {
					dest_orientation = (path_points[j + 1] - path_points[j]).orientation();
				}

				// get distance between last two points
				if (j == 0) {
					dist = (player.position() - path_points[0]).len();
				} else {
					dist = (path_points[j] - path_points[j - 1]).len();
				}

				// dribble at a different speed
				if (player.type() == AI::Flags::MoveType::DRIBBLE) {
					working_time += std::chrono::duration_cast<AI::Timediff>(std::chrono::duration<double>(dist / player.MAX_LINEAR_VELOCITY / DRIBBLE_SPEED));
				} else if (player.flags() & AI::Flags::FLAG_CAREFUL) {
					working_time += std::chrono::duration_cast<AI::Timediff>(std::chrono::duration<double>(dist / careful_max_speed));
				}

				path.push_back(std::make_pair(std::make_pair(path_points[j], dest_orientation), working_time));
			}

			player.path(path);
		}

	}
}

NAVIGATOR_REGISTER(RRTNavigator)

