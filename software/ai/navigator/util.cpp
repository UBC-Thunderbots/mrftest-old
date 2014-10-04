#include "ai/navigator/util.h"
#include "ai/flags.h"
#include "ai/navigator/rrt_planner.h"
#include "geom/rect.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/util.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <iostream>

using namespace AI::Flags;
using namespace AI::Nav::W;

namespace AI {
	namespace Nav {
		namespace RRT {
			extern IntParam jon_hysteris_hack;
		}
	}
}

namespace {
	constexpr double EPS = 1e-9;
	
	DoubleParam INTERCEPT_ANGLE_STEP_SIZE(u8"angle increment in approaching the ball, in degrees", u8"Nav/Util", 10.0, 0.1, 30.0);

	BoolParam OWN_HALF_OVERRIDE(u8"enforce that robots stay on own half ", u8"Nav/Util", false);

	// small value to ensure non-equivilance with floating point math
	// but too small to make a difference in the actual game
	constexpr double SMALL_BUFFER = 0.0001;

	BoolParam USE_ENEMY_MOVEMENT_FACTOR(u8"use enemy's future position", u8"Nav/Util", false);
	BoolParam USE_FRIENDLY_MOVEMENT_FACTOR(u8"use friendly's future position", u8"Nav/Util", false);

	DoubleParam ENEMY_MOVEMENT_FACTOR(u8"amount of time (s) to avoid enemy's future position", u8"Nav/Util", 0.0, 0.0, 2.0);
	DoubleParam FRIENDLY_MOVEMENT_FACTOR(u8"amount of time (s) to avoid friendly's future position", u8"Nav/Util", 0.0, 0.0, 2.0);
	DoubleParam GOAL_POST_BUFFER(u8"The amount robots should stay away from goal post", u8"Nav/Util", 0.0, -0.2, 0.2);
	DoubleParam ROBOT_NET_ALLOWANCE(u8"The amount centre of robot must stay out of net", u8"Nav/Util", 0.0, -0.1, 0.2);
	// zero lets them brush
	// positive enforces amount meters away
	// negative lets them bump
	// const double ENEMY_BUFFER = 0.1;
	DoubleParam ENEMY_BUFFER_SHORT(u8"The amount of distance to maintain from enemy robots that we are willing to bump", u8"Nav/Util", -0.05, -1.0, 1.0);
	DoubleParam ENEMY_BUFFER(u8"The amount of distance to maintain from enemy robots usually", u8"Nav/Util", 0.1, -1.0, 1.0);
	DoubleParam ENEMY_BUFFER_LONG(u8"The amount of distance to maintain from enemy robots of high priority", u8"Nav/Util", 0.2, -1.0, 1.0);
	// zero lets them brush
	// positive enforces amount meters away
	// negative lets them bump
	DoubleParam FRIENDLY_BUFFER(u8"Buffer for equal priority friendly robot (meters)", u8"Nav/Util", 0.1, -1.0, 1.0);
	DoubleParam FRIENDLY_BUFFER_HIGH(u8"Buffer for higher priority friendly robot (meters)", u8"Nav/Util", 0.2, -1.0, 1.0);
	DoubleParam FRIENDLY_BUFFER_LOW(u8"Buffer for lower priority friendly robot (meters)", u8"Nav/Util", 0.1, -1.0, 1.0);
	DoubleParam PASS_CHALLENGE_BUFFER(u8"Buffer for friendly robots in the pass intercept challenge (meters)", u8"Nav/Util", 1.0, 0.1, 2.0);

	// This buffer is in addition to the robot radius
	DoubleParam BALL_TINY_BUFFER(u8"Buffer avoid ball tiny (meters)", u8"Nav/Util", 0.05, -1.0, 1.0);

	// This buffer is in addition to the robot radius
	DoubleParam DEFENSE_AREA_BUFFER(u8"Buffer avoid defense area (meters)", u8"Nav/Util", 0.0, -1.0, 1.0);

	// this is by how much we should stay away from the playing boundry
	DoubleParam PLAY_AREA_BUFFER(u8"Buffer for staying away from play area boundary ", u8"Nav/Util", 0.0, 0.0, 1.0);
	DoubleParam OWN_HALF_BUFFER(u8"Buffer for staying on own half ", u8"Nav/Util", 0.0, 0.0, 1.0);
	DoubleParam TOTAL_BOUNDS_BUFFER(u8"Buffer for staying away from referee area boundary ", u8"Nav/Util", -0.18, -1.0, 1.0);

	DoubleParam PENALTY_KICK_BUFFER(u8"Amount behind ball during Penalty kick (rule=0.4) ", u8"Nav/Util", 0.4, 0.0, 1.0);

	DoubleParam FRIENDLY_KICK_BUFFER(u8"Additional offense area buffer for friendly kick (rule=0.2) ", u8"Nav/Util", 0.2, 0.0, 1.0);

	constexpr double RAM_BALL_ALLOWANCE = 0.05;

	constexpr double BALL_STOP = 0.05;
	// distance from the ball's future position before we start heading towards the ball
	constexpr double CATCH_BALL_THRESHOLD = 0.1;
	// distance behind the ball's future position that we should aim for when catching the ball
	constexpr double CATCH_BALL_DISTANCE_AWAY = 0.1;
	// if the ball velocity is below this value then act as if it isn't moving
	constexpr double CATCH_BALL_VELOCITY_THRESH = 0.05;

	// this structure determines how far away to stay from a prohibited point or line-segment
	double play_area() {
		return /*(2 * AI::Nav::W::Player::MAX_RADIUS)*/ + PLAY_AREA_BUFFER;
	}
	double total_bounds_area() {
		return TOTAL_BOUNDS_BUFFER;
	}
	double enemy(AI::Nav::W::World world, AI::Nav::W::Robot player) {
		if (world.enemy_team().size() <= 0) {
			return 0.0;
		}
		double buffer = 0.0;

		switch (player.avoid_distance()) {
			case AvoidDistance::SHORT:
				buffer = ENEMY_BUFFER_SHORT;
				break;

			case AvoidDistance::MEDIUM:
				buffer = ENEMY_BUFFER;
				break;

			case AvoidDistance::LONG:
				buffer = ENEMY_BUFFER_LONG;
				break;
		}

		return player.MAX_RADIUS + buffer;
	}

	double goal_post(AI::Nav::W::Player player) {
		return Ball::RADIUS + player.MAX_RADIUS + GOAL_POST_BUFFER;
	}

	double friendly(AI::Nav::W::Player player, MovePrio obs_prio = MovePrio::MEDIUM) {
		MovePrio player_prio = player.prio();
		double buffer = FRIENDLY_BUFFER;
		if (obs_prio < player_prio) {
			buffer = FRIENDLY_BUFFER_LOW;
		} else if (player_prio < obs_prio) {
			buffer = FRIENDLY_BUFFER_HIGH;
		}

		if(player.avoid_distance() == AvoidDistance::LONG) {
			buffer = PASS_CHALLENGE_BUFFER;
		}

		return player.MAX_RADIUS + buffer;
	}
	double ball_stop(AI::Nav::W::Player player) {
		return Ball::RADIUS + player.MAX_RADIUS + AI::Util::BALL_STOP_DIST;
	}
	double ball_tiny(AI::Nav::W::Player player) {
		return Ball::RADIUS + player.MAX_RADIUS + BALL_TINY_BUFFER;
	}
	double friendly_defense(AI::Nav::W::World world, AI::Nav::W::Player player) {
		return world.field().defense_area_radius() + player.MAX_RADIUS + DEFENSE_AREA_BUFFER;
	}
	double friendly_kick(AI::Nav::W::World world, AI::Nav::W::Player player) {
		return world.field().defense_area_radius() + player.MAX_RADIUS + FRIENDLY_KICK_BUFFER;
	}
	double own_half(AI::Nav::W::Player player) {
		return player.MAX_RADIUS + OWN_HALF_BUFFER;
	}
	double penalty_kick_friendly(AI::Nav::W::Player player) {
		return player.MAX_RADIUS + PENALTY_KICK_BUFFER + Ball::RADIUS;
	}
	double penalty_kick_enemy(AI::Nav::W::Player player) {
		return player.MAX_RADIUS + PENALTY_KICK_BUFFER + Ball::RADIUS;
	}

	double get_net_trespass(Point cur, Point dst, AI::Nav::W::World world) {
		double violate = 0.0;
		double circle_radius = world.field().total_length() / 2.0 - world.field().length() / 2.0;
		Point A(-world.field().total_length() / 2.0, world.field().goal_width() / 2.0);
		Point B(-world.field().total_length() / 2.0, -world.field().goal_width() / 2.0);
		double dist = seg_seg_distance(A, B, cur, dst);
		violate = std::max(violate, circle_radius - dist);
		return violate;
	}

	double get_goal_post_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
		double violate = 0.0;
		double circle_radius = goal_post(player);
		Point A(world.field().length() / 2.0, world.field().goal_width() / 2.0);
		Point B(world.field().length() / 2.0, -world.field().goal_width() / 2.0);
		Point C(-world.field().length() / 2.0, world.field().goal_width() / 2.0);
		Point D(-world.field().length() / 2.0, -world.field().goal_width() / 2.0);
		Point pts[4] = { A, B, C, D };
		for (Point i : pts) {
			double dist = lineseg_point_dist(i, cur, dst);
			violate = std::max(violate, circle_radius - dist);
		}
		return violate;
	}

	double get_enemy_trespass(Point cur, Point dst, AI::Nav::W::World world) {
		double violate = 0.0;
		// avoid enemy robots
		for (AI::Nav::W::Robot rob : world.enemy_team()) {
			double circle_radius = enemy(world, rob);
			double dist = lineseg_point_dist(rob.position(), cur, dst);
			if (USE_ENEMY_MOVEMENT_FACTOR) {
				dist = seg_seg_distance(rob.position(), rob.position() + ENEMY_MOVEMENT_FACTOR * rob.velocity(), cur, dst);
			}
			violate = std::max(violate, circle_radius - dist);
		}

		return violate;
	}

	double get_play_area_boundary_trespass(Point cur, Point dst, AI::Nav::W::World world) {
		const Field &f = world.field();
		Point sw_corner(-f.length() / 2, -f.width() / 2);
		Rect bounds(sw_corner, f.length(), f.width());
		bounds.expand(-play_area());
		double violation = 0.0;
		if (!bounds.point_inside(cur)) {
			violation = std::max(violation, bounds.dist_to_boundary(cur));
		}
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	double get_total_bounds_trespass(Point cur, Point dst, AI::Nav::W::World world) {
		const Field &f = world.field();
		Point sw_corner(-f.total_length() / 2, -f.total_width() / 2);
		Rect bounds(sw_corner, f.total_length(), f.total_width());
		bounds.expand(-total_bounds_area());
		double violation = 0.0;
		if (!bounds.point_inside(cur)) {
			violation = std::max(violation, bounds.dist_to_boundary(cur));
		}
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	double get_friendly_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
		double violate = 0.0;
		// avoid enemy robots
		for (AI::Nav::W::Player rob : world.friendly_team()) {
			if (rob == player) {
				continue;
			}
			double circle_radius = friendly(player, rob.prio());
			double dist = lineseg_point_dist(rob.position(), cur, dst);
			if (USE_FRIENDLY_MOVEMENT_FACTOR) {
				dist = seg_seg_distance(rob.position(), rob.position() + FRIENDLY_MOVEMENT_FACTOR * rob.velocity(), cur, dst);
			}

			violate = std::max(violate, circle_radius - dist);
		}
		return violate;
	}

	double get_ball_stop_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
		double violate = 0.0;
		const Ball &ball = world.ball();
		double circle_radius = ball_stop(player);
		double dist = lineseg_point_dist(ball.position(), cur, dst);
		violate = std::max(violate, circle_radius - dist);
		return violate;
	}

	double get_defense_area_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
		const Field &f = world.field();
		double defense_dist = friendly_defense(world, player);
		Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
		return std::max(0.0, defense_dist - seg_seg_distance(cur, dst, defense_point1, defense_point2));
	}

	double get_own_half_trespass(Point, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
		const Field &f = world.field();
		Point p(-f.total_length() / 2, -f.total_width() / 2);
		Rect bounds(p, f.total_length() / 2, f.total_width());
		bounds.expand(-own_half(player));
		double violation = 0.0;
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	double get_offense_area_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
		const Field &f = world.field();
		double defense_dist = friendly_kick(world, player);
		Point defense_point1(f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(f.length() / 2, f.defense_area_stretch() / 2);
		return std::max(0.0, defense_dist - seg_seg_distance(cur, dst, defense_point1, defense_point2));
	}

	double get_ball_tiny_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
		const Ball &ball = world.ball();
		double circle_radius = ball_tiny(player);
		double dist = lineseg_point_dist(ball.position(), cur, dst);
		return std::max(0.0, circle_radius - dist);
	}

	double get_penalty_friendly_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
		const Ball &ball = world.ball();
		const Field &f = world.field();
		Point a(ball.position().x - penalty_kick_friendly(player), -f.total_width() / 2);
		Point b(f.total_length() / 2, f.total_width() / 2);
		Rect bounds(a, b);
		double violation = 0.0;
		if (!bounds.point_inside(cur)) {
			violation = std::max(violation, bounds.dist_to_boundary(cur));
		}
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	double get_penalty_enemy_trespass(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
		const Ball &ball = world.ball();
		const Field &f = world.field();
		Point a(ball.position().x + penalty_kick_enemy(player), -f.total_width() / 2);
		Point b(f.total_length() / 2, f.total_width() / 2);
		Rect bounds(a, b);
		double violation = 0.0;
		if (!bounds.point_inside(cur)) {
			violation = std::max(violation, bounds.dist_to_boundary(cur));
		}
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	struct violation {
		double enemy, friendly, play_area, ball_stop, ball_tiny, friendly_defense, enemy_defense, own_half, penalty_kick_friendly, penalty_kick_enemy, goal_post, total_bounds, net_allowance;

		unsigned int extra_flags;

		// set the amount of violation that the player currently has
		void set_violation_amount(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
			friendly = get_friendly_trespass(cur, dst, world, player);
			enemy = get_enemy_trespass(cur, dst, world);
			goal_post = get_goal_post_trespass(cur, dst, world, player);
			total_bounds = get_total_bounds_trespass(cur, dst, world);
			net_allowance = get_net_trespass(cur, dst, world);
			unsigned int flags = player.flags() | extra_flags;

			if (flags & FLAG_CLIP_PLAY_AREA) {
				play_area = get_play_area_boundary_trespass(cur, dst, world);
			}
			if (flags & FLAG_AVOID_BALL_STOP) {
				ball_stop = get_ball_stop_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_AVOID_BALL_TINY) {
				ball_tiny = get_ball_tiny_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_AVOID_FRIENDLY_DEFENSE) {
				friendly_defense = get_defense_area_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_AVOID_ENEMY_DEFENSE) {
				friendly_defense = get_offense_area_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_STAY_OWN_HALF) {
				own_half = get_own_half_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_PENALTY_KICK_FRIENDLY) {
				penalty_kick_friendly = get_penalty_friendly_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_PENALTY_KICK_ENEMY) {
				penalty_kick_enemy = get_penalty_enemy_trespass(cur, dst, world, player);
			}
		}

		// default, no violation
		explicit violation() : enemy(0.0), friendly(0.0), play_area(0.0), ball_stop(0.0), ball_tiny(0.0), friendly_defense(0.0), enemy_defense(0.0), own_half(0.0), penalty_kick_friendly(0.0), penalty_kick_enemy(0.0), goal_post(0.0), total_bounds(0.0), net_allowance(0.0), extra_flags(0) {
		}

		explicit violation(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) : enemy(0.0), friendly(0.0), play_area(0.0), ball_stop(0.0), ball_tiny(0.0), friendly_defense(0.0), enemy_defense(0.0), own_half(0.0), penalty_kick_friendly(0.0), penalty_kick_enemy(0.0), goal_post(0.0), total_bounds(0.0), net_allowance(0.0), extra_flags(0) {
			if (OWN_HALF_OVERRIDE) {
				extra_flags = extra_flags | FLAG_STAY_OWN_HALF;
			}
			set_violation_amount(cur, dst, world, player);
		}

		explicit violation(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player, unsigned int added_flags) : enemy(0.0), friendly(0.0), play_area(0.0), ball_stop(0.0), ball_tiny(0.0), friendly_defense(0.0), enemy_defense(0.0), own_half(0.0), penalty_kick_friendly(0.0), penalty_kick_enemy(0.0), goal_post(0.0), total_bounds(0.0), net_allowance(0.0), extra_flags(added_flags) {
			if (OWN_HALF_OVERRIDE) {
				extra_flags = extra_flags | FLAG_STAY_OWN_HALF;
			}
			set_violation_amount(cur, dst, world, player);
		}

		static violation get_violation_amount(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
			violation v(cur, dst, world, player);
			return v;
		}

		static violation get_violation_amount(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player, unsigned int extra_flags) {
			violation v(cur, dst, world, player, extra_flags);
			return v;
		}

		// whether there is less violation than the violation parameter
		bool no_more_violating_than(violation b) {
			return enemy < b.enemy + EPS && friendly < b.friendly + EPS &&
			       play_area < b.play_area + EPS && ball_stop < b.ball_stop + EPS &&
			       ball_tiny < b.ball_tiny + EPS && friendly_defense < b.friendly_defense + EPS &&
			       enemy_defense < b.enemy_defense + EPS && own_half < b.own_half + EPS &&
			       penalty_kick_enemy < b.penalty_kick_enemy + EPS &&
			       penalty_kick_friendly < b.penalty_kick_friendly + EPS &&
			       goal_post < b.goal_post + EPS &&
			       total_bounds < b.total_bounds + EPS &&
			       net_allowance < b.net_allowance + EPS;
		}

		// whether there are no violations at all
		bool violation_free() {
			return enemy < EPS && friendly < EPS &&
			       play_area < EPS && ball_stop < EPS &&
			       ball_tiny < EPS && friendly_defense < EPS &&
			       enemy_defense < EPS && own_half < EPS &&
			       penalty_kick_enemy < EPS && penalty_kick_friendly < EPS &&
			       goal_post < EPS && total_bounds < EPS &&
			       net_allowance < EPS;
		}
	};


	void process_obstacle(std::vector<Point> &ans, AI::Nav::W::World world, AI::Nav::W::Player player, Point segA, Point segB, double dist, int num_points) {
		// we want a regular polygon where the largest inscribed circle
		// has the keepout distance as it's radius
		// circle radius then becomes the radius of the smallest circle that will contain the polygon
		// plus a small buffer
		double radius = dist / std::cos(M_PI / static_cast<double>(num_points)) + SMALL_BUFFER;
		double TS = 2 * num_points * dist * std::tan(M_PI / num_points);
		double TS2 = TS + 2 * (segA - segB).len();
		int n_tot = num_points * static_cast<int>(std::ceil(TS2 / TS));
		std::vector<Point> temp = seg_buffer_boundaries(segA, segB, radius, n_tot);

		for (Point i : temp) {
			if (AI::Nav::Util::valid_dst(i, world, player)) {
				ans.push_back(i);
			}
		}
	}

	// create path that avoids obstacles and just keeps the orientation constant at each of the points
	AI::Nav::W::Player::Path create_path(AI::Nav::W::World world, AI::Nav::W::Player player, const std::pair<Point, Angle> dest, unsigned int added_flags) {
		std::vector<Point> path_points;
		AI::Nav::RRTPlanner planner(world);
		AI::Nav::W::Player::Path path;

		if (AI::Nav::Util::valid_path(player.position(), dest.first, world, player, added_flags)) {
			path_points.push_back(dest.first);
		} else {
			path_points = planner.plan(player, dest.first, added_flags);
		}

		for (Point i : path_points) {
			path.push_back(std::make_pair(std::make_pair(i, dest.second), world.monotonic_time()));
		}
		return path;
	}

	// Get a path that goes beside the ball
	AI::Nav::W::Player::Path get_path_near_ball(AI::Nav::W::World world, const AI::Nav::W::Player player, const Angle &target_ball_offset_angle) {
		Point dest_pos;
		Angle dest_ang;
		const Point ball_vel = world.ball().velocity();
		const Point ball_norm = world.ball().velocity().norm();
		const double AVOID_DIST = 0.2;
		const double DELTA_TIME = 1.0;
		// check for the side that has a clear path, TODO

		// if the robot is in behind the ball, then move up to it from the far side to the target
		if (target_ball_offset_angle.to_degrees() > 0.0) {
			dest_pos = world.ball().position() + ball_norm.rotate(Angle::quarter()) * AVOID_DIST + ball_vel * DELTA_TIME;
		} else {
			dest_pos = world.ball().position() + ball_norm.rotate(Angle::three_quarter()) * AVOID_DIST + ball_vel * DELTA_TIME;
		}

		// make the robot face against the ball direction
		dest_ang = -ball_vel.orientation();

		return create_path(world, player, std::make_pair(dest_pos, dest_ang), AI::Flags::FLAG_AVOID_BALL_TINY);
	}

	// only used when ball is not moving
	AI::Nav::W::Player::Path get_path_around_ball(AI::Nav::W::World world, const AI::Nav::W::Player player, const Point player_pos, const Point target_pos, bool ccw) {
		Point dest_pos;
		Angle dest_ang = (target_pos - player_pos).orientation();
		const Point ball_pos = world.ball().position();

		Point radial_norm = (player_pos - ball_pos).norm();
		Angle angle_diff = (radial_norm.orientation() - (Angle::half() + dest_ang).angle_mod()).angle_mod();
		ccw = angle_diff > Angle::zero();

		// when we are rotating, angle the movement inward slightly by this amount
		// this is because we start rotating around the ball at a certain distance and can move slightly out of that distance
		// causing the robot to attempt to correct and not be able to grab the ball smoothly
		Angle rotate_offset_angle = Angle::of_degrees(10);
		Point tangential_norm;
		if (ccw) {
			tangential_norm = radial_norm.rotate(Angle::three_quarter() - rotate_offset_angle);
		} else {
			tangential_norm = radial_norm.rotate(Angle::quarter() + rotate_offset_angle);
		}

		const double tangential_scale = 0.2;
		if (angle_diff.angle_diff(Angle::zero()) > Angle::of_degrees(10)) {
			dest_pos = player_pos + tangential_norm * tangential_scale;
		} else {
			dest_pos = ball_pos;
		}

		return create_path(world, player, std::make_pair(dest_pos, dest_ang), 0);
	}
};

std::vector<Point> AI::Nav::Util::get_destination_alternatives(Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
	const int POINTS_PER_OBSTACLE = 6;
	std::vector<Point> ans;
	unsigned int flags = player.flags();

	if (flags & FLAG_AVOID_BALL_STOP) {
		process_obstacle(ans, world, player, dst, dst, friendly(player), 3 * POINTS_PER_OBSTACLE);
	}

	return ans;
}

bool AI::Nav::Util::valid_dst(Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
	return violation::get_violation_amount(dst, dst, world, player).violation_free();
}

bool AI::Nav::Util::valid_path(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
	return violation::get_violation_amount(cur, dst, world, player).no_more_violating_than(violation::get_violation_amount(cur, cur, world, player));
}

bool AI::Nav::Util::valid_path(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player, unsigned int extra_flags) {
	return violation::get_violation_amount(cur, dst, world, player, extra_flags).no_more_violating_than(violation::get_violation_amount(cur, cur, world, player, extra_flags));
}

std::vector<Point> AI::Nav::Util::get_obstacle_boundaries(AI::Nav::W::World world, AI::Nav::W::Player player) {
	return get_obstacle_boundaries(world, player, 0);
}
#warning some magic numbers here need to clean up a bit
std::vector<Point> AI::Nav::Util::get_obstacle_boundaries(AI::Nav::W::World world, AI::Nav::W::Player player, unsigned int added_flags) {
	// this number must be >=3
	const int POINTS_PER_OBSTACLE = 6;
	std::vector<Point> ans;
	unsigned int flags = player.flags() | added_flags;
	const Field &f = world.field();

	if (flags & FLAG_AVOID_BALL_STOP) {
		process_obstacle(ans, world, player, world.ball().position(), world.ball().position(), ball_stop(player), 3 * POINTS_PER_OBSTACLE);
	}

	if (flags & FLAG_STAY_OWN_HALF) {
		Point half_point1(0.0, -f.width() / 2);
		Point half_point2(0.0, f.width() / 2);
		process_obstacle(ans, world, player, half_point1, half_point2, own_half(player), 7 * POINTS_PER_OBSTACLE);
	}

	if (flags & FLAG_AVOID_FRIENDLY_DEFENSE) {
		Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
		process_obstacle(ans, world, player, defense_point1, defense_point2, friendly_defense(world, player), POINTS_PER_OBSTACLE);
	}

	if (flags & FLAG_AVOID_ENEMY_DEFENSE) {
		Point defense_point1(f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(f.length() / 2, f.defense_area_stretch() / 2);
		process_obstacle(ans, world, player, defense_point1, defense_point2, friendly_kick(world, player), POINTS_PER_OBSTACLE);
	}

	if ((flags & FLAG_AVOID_BALL_TINY) && !(flags & FLAG_AVOID_BALL_STOP)) {
		process_obstacle(ans, world, player, world.ball().position(), world.ball().position(), ball_tiny(player), POINTS_PER_OBSTACLE);
	}

	for (AI::Nav::W::Player rob : world.friendly_team()) {
		if (rob == player) {
			// points around self may help with trying to escape when stuck
			// that is why there are double the number of points here
			process_obstacle(ans, world, player, rob.position(), rob.position(), friendly(player), 2 * POINTS_PER_OBSTACLE);
			continue;
		}
		process_obstacle(ans, world, player, rob.position(), rob.position(), friendly(player), POINTS_PER_OBSTACLE);
	}

	for (AI::Nav::W::Robot rob : world.enemy_team()) {
		process_obstacle(ans, world, player, rob.position(), rob.position(), enemy(world, player), POINTS_PER_OBSTACLE);
	}

	return ans;
}

std::pair<Point, AI::Timestamp> AI::Nav::Util::get_ramball_location(Point dst, AI::Nav::W::World world, AI::Nav::W::Player player) {
	Point ball_dir = world.ball().velocity();

	if (ball_dir.lensq() < EPS) {
		if (lineseg_point_dist(world.ball().position(), player.position(), dst) < RAM_BALL_ALLOWANCE) {
			return std::make_pair(world.ball().position(), world.monotonic_time());
		}
	}

	if (unique_line_intersect(player.position(), dst, world.ball().position(), world.ball().position() + ball_dir)) {
		Point location = line_intersect(player.position(), dst, world.ball().position(), world.ball().position() + ball_dir);
		AI::Timestamp intersect = world.monotonic_time();
		intersect += std::chrono::duration_cast<AI::Timediff>(std::chrono::duration<double>((location - world.ball().position()).len() / world.ball().velocity().len()));

		Point vec1 = location - player.position();
		Point vec2 = dst - player.position();

		Point ball_vec = location - world.ball().position();

		if (vec1.dot(vec2) > 0 && ball_dir.dot(ball_vec) != 0.0) {
			return std::make_pair(location, intersect);
		}
	}

	// if everything fails then just stay put
	return std::make_pair(player.position(), world.monotonic_time());
}

AI::Timestamp AI::Nav::Util::get_next_ts(AI::Timestamp now, Point &p1, Point &p2, Point target_velocity) {
	double velocity, distance;
	velocity = target_velocity.len();
	distance = (p1 - p2).len();
#warning unit cancellation says this is not time, should be distance divided by velocity!
	return now + std::chrono::duration_cast<AI::Timediff>(std::chrono::duration<double>(velocity * distance));
}

double AI::Nav::Util::estimate_action_duration(std::vector<std::pair<Point, Angle>> path_points) {
	double total_time = 0;
	for (std::size_t i = 0; i < path_points.size() - 1; ++i) {
		double dist = (path_points[i].first - path_points[i + 1].first).len();
		total_time += dist / Player::MAX_LINEAR_VELOCITY;
	}
	return total_time;
}

bool AI::Nav::Util::intercept_flag_stationary_ball_handler(AI::Nav::W::World world, AI::Nav::W::Player player){
	const Ball &ball = world.ball();

	const Point ball_pos = ball.position();
	const Point bot_pos = player.position();
	const Point target_pos = player.destination().first;

	const Point radial_dist = bot_pos - ball_pos;
	const Angle target_angle = ((target_pos - ball_pos).orientation()+Angle::of_degrees(180)).angle_mod();
	const Angle bot_angle = (bot_pos - ball_pos).orientation().angle_mod();
	const Angle angular_dist = (bot_angle-target_angle).angle_mod();

	// the direction that robot should eventually face
	const Angle target_orient = (target_pos-ball_pos).orientation().angle_mod();
	
	// number of step in the path
	const int seg_number = int(std::floor(std::abs(angular_dist/Angle::of_degrees(INTERCEPT_ANGLE_STEP_SIZE)))+1);
	// how far the step travels radially, 
	const Point radial_step = radial_dist/(seg_number+1);
	// how far the step tarvels tangentially
	const Angle angular_step = angular_dist/seg_number;

	//std::cout << "step number " << seg_number << " angular_step " << angular_step.to_degrees() << " angular_dist " << angular_dist.to_degrees() << " target_angle " << target_angle.to_degrees() << "\n";
	
	std::vector<Point> step_points;
	// i starts with 1 because the first step point is not robot position
	for (int i = 1; i < seg_number; i++) {
		Point radial_pos = Point((radial_dist-radial_step*i).len(),0);
		Angle angular_pos = bot_angle-angular_step*i;
		// in every step robot get closer to the ball by 1 radial_step, and get closer lining up the robot with target by 1 angular_step
		step_points.push_back(ball_pos+radial_pos.rotate(angular_pos));
	}
	// last point is ball position
	step_points.push_back(ball_pos);

	AI::Nav::W::Player::Path path;
	AI::Timestamp working_time = world.monotonic_time();

	for (Point i : step_points) {
		path.push_back(std::make_pair(std::make_pair(i, target_orient), working_time));
	}
	player.path(path);
	return true;
}

bool AI::Nav::Util::intercept_flag_handler(AI::Nav::W::World world, AI::Nav::W::Player player,  AI::Nav::RRT::PlayerData::Ptr player_data) {
	// need to confirm that the player has proper flag

	// need to confirm that the ball is moving at all

	// extract data from the player
	const Field &field = world.field();
	Point target_pos = player.destination().first;
	const Point robot_pos = player.position();

	Angle dest_orientation = player.destination().second;
	// extract information from the ball
	const Ball &ball = world.ball();
	const Rect field_rec({ field.length() / 2, field.width() / 2 }, { -field.length() / 2, -field.width() / 2 });
	const Point ball_pos = ball.position();
	const Point ball_vel = ball.velocity();
	const Point ball_norm = ball_vel.norm();
	const Point ball_bounded_pos = vector_rect_intersect(field_rec, ball_pos, ball_pos + ball_norm);
	const Angle target_ball_offset_angle = vertex_angle(ball_pos + ball_vel, ball_pos, target_pos).angle_mod();

	if (player_data->prev_move_type == player.type() && player_data->prev_move_prio == player.prio() && player_data->prev_avoid_distance == player.avoid_distance()) {
		target_pos = (player_data->previous_dest + target_pos ) / AI::Nav::RRT::jon_hysteris_hack;
		player_data->previous_dest = target_pos;
		dest_orientation = (player_data->previous_orient + dest_orientation) / AI::Nav::RRT::jon_hysteris_hack;
		player_data->previous_orient = dest_orientation;
	}


	std::vector<Point> path_points;
	AI::Nav::RRTPlanner planner(world);

//	if (ball_vel.len() < 0.2) {
//		//player.path(get_path_around_ball(world, player, robot_pos, target_pos, true));
//		intercept_flag_stationary_ball_handler(world, player);
//		return true;
//	}

	// only start rotating around the stationary ball when we're within a certain distance
	const double dist_to_rotate = 0.25;
	if (ball_vel.len() < 0.4 && (robot_pos - ball_pos).len() < dist_to_rotate) {
		player.path(get_path_around_ball(world, player, robot_pos, target_pos, true));
		return true;
	}

	const bool robot_behind_ball = !point_in_front_vector(ball_pos, ball_vel, robot_pos);
	// find out whether robot is behind the ball or in front of the ball
	if (robot_behind_ball) {
		player.path(get_path_near_ball(world, player, target_ball_offset_angle));
		return true;
	}

	// if the intersection is off the field or not found for some reason and the ball is not moving very slow, return failure
	if ((ball.velocity().len() > CATCH_BALL_VELOCITY_THRESH) && ball_bounded_pos.x == 0.0 && ball_bounded_pos.y == 0.0) {
		return false;
	}

	// set up the resolution that we should check at
	int points_to_check = 60;
	Point interval = (-ball_pos + ball_bounded_pos) * (1.0 / points_to_check);
	// set up how much the ball travels in each interval that we check, assume no decay
	double interval_time = interval.len() / ball.velocity().len();
	unsigned int flags = AI::Flags::FLAG_AVOID_BALL_TINY;

#warning flags and timespec are not accounted for properly
	for (int i = 0; i <= points_to_check; i++) {
		// where the ball would roll to at a later time
		Point ball_future_pos = ball_pos + (interval * i);
		Point dir_from_target = (ball_future_pos - player.destination().first).norm();

		// get a point that is behind the ball's future position in the direction of the target, this is where the robot should go to
		Point move_to_point = ball_future_pos + (dir_from_target * CATCH_BALL_DISTANCE_AWAY);

		// now plan out the path
		path_points = planner.plan(player, move_to_point, flags);
		std::vector<std::pair<Point, Angle>> path_points_with_angle;

		// face the final direction the whole time
		Angle path_orientation = (dir_from_target * -1).orientation();
		path_points_with_angle.push_back(std::make_pair(player.position(), path_orientation));
		for (Point j : path_points) {
			path_points_with_angle.push_back(std::make_pair(j, path_orientation));
		}

		// check if the robot can make it
		if (AI::Nav::Util::estimate_action_duration(path_points_with_angle) < (interval_time * i) || (i == points_to_check) || ball.velocity().len() < CATCH_BALL_VELOCITY_THRESH) {
			// prepare for assigning the path to the robot
			AI::Nav::W::Player::Path path;
			AI::Timestamp working_time = world.monotonic_time();

			// if we're within a certain threshold then skip this and just move towards the ball's future position
			if (line_pt_dist(move_to_point, ball_future_pos, player.position()) > CATCH_BALL_THRESHOLD) {
				// ignore first point since it is bot's position
				for (unsigned int j = 1; j < path_points_with_angle.size(); j++) {
					// not going for proper timestamp, yet
					path.push_back(std::make_pair(path_points_with_angle[j], working_time));
				}
			}

			// add last point to be the actual point where the ball should be since we generate a path to a point just behind it
			path.push_back(std::make_pair(std::make_pair(ball_future_pos, player.destination().second), working_time));
			player.path(path);
			return true;
		}
	}

	// guess we have't found a possible intersecting point
	return false;
}

