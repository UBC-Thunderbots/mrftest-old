#include "ai/navigator/util.h"
#include "ai/flags.h"
#include <algorithm>
#include <cmath>
#include <vector>

using namespace AI::Flags;
using namespace AI::Nav::W;

namespace {
	const double EPS = 1e-9;

	// small value to ensure non-equivilance with floating point math
	// but too small to make a difference in the actual game
	const double SMALL_BUFFER = 0.0001;

	// zero lets them brush
	// positive enforces amount meters away
	// negative lets them bump
	const double ENEMY_BUFFER = 0.0;

	// zero lets them brush
	// positive enforces amount meters away
	// negative lets them bump
	const double FRIENDLY_BUFFER = 0.0;

	// This buffer is in addition to the robot radius
	const double BALL_STOP_BUFFER = 0.5;

	// This buffer is in addition to the robot radius
	const double BALL_TINY_BUFFER = 0.05;

	// This buffer is in addition to the robot radius
	const double DEFENSE_AREA_BUFFER = 0.0;

	// this is by how much we should stay away from the playing boundry
	const double PLAY_AREA_BUFFER = 0.0;

	const double OWN_HALF_BUFFER = 0.0;

	const double PENALTY_KICK_BUFFER = 0.4;

	const double FRIENDLY_KICK_BUFFER = 0.2;

	// this structure determines how far away to stay from a prohibited point or line-segment
	struct distance_keepout {
		static double play_area(AI::Nav::W::Player::Ptr player) {
			return (player->MAX_RADIUS) + PLAY_AREA_BUFFER;
		}
		static double enemy(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			if (world.enemy_team().size() <= 0) {
				return 0.0;
			}
			return player->MAX_RADIUS + world.enemy_team().get(0)->MAX_RADIUS + ENEMY_BUFFER;
		}
		static double friendly(AI::Nav::W::Player::Ptr player) {
			return 2.0 * (player->MAX_RADIUS) + FRIENDLY_BUFFER;
		}
		static double ball_stop(AI::Nav::W::Player::Ptr player) {
			return Ball::RADIUS + player->MAX_RADIUS + BALL_STOP_BUFFER;
		}
		static double ball_tiny(AI::Nav::W::Player::Ptr player) {
			return Ball::RADIUS + player->MAX_RADIUS + BALL_TINY_BUFFER;
		}
		static double friendly_defense(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			return world.field().defense_area_radius() + player->MAX_RADIUS + DEFENSE_AREA_BUFFER;
		}
		static double enemy_defense(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			return world.field().defense_area_radius() + player->MAX_RADIUS + DEFENSE_AREA_BUFFER;
		}

		static double own_half(AI::Nav::W::Player::Ptr player) {
			return player->MAX_RADIUS + OWN_HALF_BUFFER;
		}
		static double penalty_kick_friendly(AI::Nav::W::Player::Ptr player) {
			return player->MAX_RADIUS + PENALTY_KICK_BUFFER + Ball::RADIUS;
		}
		static double penalty_kick_enemy(AI::Nav::W::Player::Ptr player) {
			return player->MAX_RADIUS + PENALTY_KICK_BUFFER + Ball::RADIUS;
		}
		static double friendly_kick(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			return enemy_defense(world, player) + FRIENDLY_KICK_BUFFER;
		}
	};

	inline double get_enemy_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double violate = 0.0;
		double circle_radius = distance_keepout::enemy(world, player);
		// avoid enemy robots
		for (unsigned int i = 0; i < world.enemy_team().size(); i++) {
			AI::Nav::W::Robot::Ptr rob = world.enemy_team().get(i);
			double dist = lineseg_point_dist(rob->position(), cur, dst);
			violate = std::max(violate, circle_radius - dist);
		}
		return violate;
	}

	inline double get_play_area_boundary_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		Point sw_corner(f.length() / 2, f.width() / 2);
		Rect bounds(sw_corner, f.length(), f.width());
		bounds.expand(-distance_keepout::play_area(player));
		double violation = 0.0;
		if (!bounds.point_inside(cur)) {
			violation = std::max(violation, bounds.dist_to_boundary(cur));
		}
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	inline double get_friendly_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double player_rad = player->MAX_RADIUS;
		double violate = 0.0;
		// avoid enemy robots
		for (unsigned int i = 0; i < world.friendly_team().size(); i++) {
			AI::Nav::W::Player::Ptr rob = world.friendly_team().get(i);
			if (rob == player) {
				continue;
			}
			double friendly_rad = rob->MAX_RADIUS;
			double circle_radius = player_rad + friendly_rad + FRIENDLY_BUFFER;
			double dist = lineseg_point_dist(rob->position(), cur, dst);
			violate = std::max(violate, circle_radius - dist);
		}
		return violate;
	}

	inline double get_ball_stop_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double violate = 0.0;
		const Ball &ball = world.ball();
		double circle_radius = distance_keepout::ball_stop(player);
		double dist = lineseg_point_dist(ball.position(), cur, dst);
		violate = std::max(violate, circle_radius - dist);
		return violate;
	}

	inline double get_defense_area_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		double defense_dist = distance_keepout::friendly_defense(world, player);
		Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
		return std::max(0.0, defense_dist - seg_seg_distance(cur, dst, defense_point1, defense_point2));
	}

	inline double get_friendly_kick_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		double defense_dist = distance_keepout::friendly_kick(world, player);
		Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
		return std::max(0.0, defense_dist - seg_seg_distance(cur, dst, defense_point1, defense_point2));
	}

	inline double get_own_half_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		Point p(-f.total_length() / 2, -f.total_width() / 2);
		Rect bounds(p, f.total_length() / 2, f.total_width());
		bounds.expand(-distance_keepout::own_half(player));
		double violation = 0.0;
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}
	inline double get_offense_area_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		double defense_dist = distance_keepout::friendly_defense(world, player);
		Point defense_point1(f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(f.length() / 2, f.defense_area_stretch() / 2);
		return std::max(0.0, defense_dist - seg_seg_distance(cur, dst, defense_point1, defense_point2));
	}

	inline double get_ball_tiny_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Ball &ball = world.ball();
		double circle_radius = distance_keepout::ball_tiny(player);
		double dist = lineseg_point_dist(ball.position(), cur, dst);
		return std::max(0.0, circle_radius - dist);
	}

	inline double get_penalty_friendly_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Ball &ball = world.ball();
		const Field &f = world.field();
		Point a(ball.position().x - distance_keepout::penalty_kick_friendly(player), -f.total_width() / 2);
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

	inline double get_penalty_enemy_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Ball &ball = world.ball();
		const Field &f = world.field();
		Point a(ball.position().x + distance_keepout::penalty_kick_enemy(player), -f.total_width() / 2);
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
		double enemy, friendly, play_area, ball_stop, ball_tiny, friendly_defense, enemy_defense, own_half, penalty_kick_friendly, penalty_kick_enemy, friendly_kick;

		unsigned int extra_flags;

		void set_violation_amount(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			friendly = get_friendly_trespass(cur, dst, world, player);
			enemy = get_enemy_trespass(cur, dst, world, player);
			unsigned int flags = player->flags() | extra_flags;
			if (flags & FLAG_CLIP_PLAY_AREA) {
				play_area = get_play_area_boundary_trespass(cur, dst, world, player);
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
			if (flags & FLAG_FRIENDLY_KICK) {
				friendly_kick = get_friendly_kick_trespass(cur, dst, world, player);
			}
		}

		violation() : enemy(0.0), friendly(0.0), play_area(0.0), ball_stop(0.0), ball_tiny(0.0), friendly_defense(0.0), enemy_defense(0.0), own_half(0.0), penalty_kick_friendly(0.0), penalty_kick_enemy(0.0), friendly_kick(0.0), extra_flags(0) {
		}

		violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) : enemy(0.0), friendly(0.0), play_area(0.0), ball_stop(0.0), ball_tiny(0.0), friendly_defense(0.0), enemy_defense(0.0), own_half(0.0), penalty_kick_friendly(0.0), penalty_kick_enemy(0.0), friendly_kick(0.0), extra_flags(0) {
			set_violation_amount(cur, dst, world, player);
		}

		violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int added_flags) : enemy(0.0), friendly(0.0), play_area(0.0), ball_stop(0.0), ball_tiny(0.0), friendly_defense(0.0), enemy_defense(0.0), own_half(0.0), penalty_kick_friendly(0.0), penalty_kick_enemy(0.0), friendly_kick(0.0), extra_flags(added_flags) {
			set_violation_amount(cur, dst, world, player);
		}

		static violation get_violation_amount(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			violation v(cur, dst, world, player);
			return v;
		}

		static violation get_violation_amount(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int extra_flags) {
			violation v(cur, dst, world, player, extra_flags);
			return v;
		}

		bool no_more_violating_than(violation b) {
			return enemy < b.enemy + EPS && friendly < b.friendly + EPS &&
			       play_area < b.play_area + EPS && ball_stop < b.ball_stop + EPS &&
			       ball_tiny < b.ball_tiny + EPS && friendly_defense < b.friendly_defense + EPS &&
			       enemy_defense < b.enemy_defense + EPS && own_half < b.own_half + EPS &&
			       penalty_kick_enemy < b.penalty_kick_enemy + EPS && penalty_kick_friendly < b.penalty_kick_friendly + EPS;
		}

		bool violation_free() {
			return enemy < EPS && friendly < EPS &&
			       play_area < EPS && ball_stop < EPS &&
			       ball_tiny < EPS && friendly_defense < EPS &&
			       enemy_defense < EPS && own_half < EPS &&
			       penalty_kick_enemy < EPS && penalty_kick_friendly < EPS;
		}
	};


	void process_obstacle(std::vector<Point> &ans, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, Point segA, Point segB, double dist, int num_points) {
		// we want a regular polygon where the largest inscribed circle
		// has the keepout distance as it's radius
		// circle radius then becomes the radius of the smallest circle that will contain the polygon
		// plus a small buffer
		double radius = dist / std::cos(M_PI / static_cast<double>(num_points)) + SMALL_BUFFER;
		double TS = 2 * num_points * dist * std::tan(M_PI / num_points);
		double TS2 = TS + 2 * (segA - segB).len();
		int n_tot = num_points * static_cast<int>(std::ceil(TS2 / TS));
		std::vector<Point> temp = seg_buffer_boundaries(segA, segB, radius, n_tot);

		for (std::vector<Point>::const_iterator it = temp.begin(); it != temp.end(); it++) {
			if (AI::Nav::Util::valid_dst(*it, world, player)) {
				ans.push_back(*it);
			}
		}
	}
};

std::vector<Point> AI::Nav::Util::get_destination_alternatives(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	const int POINTS_PER_OBSTACLE = 6;
	std::vector<Point> ans;
	unsigned int flags = player->flags();
	const Field &f = world.field();

	if (flags & FLAG_AVOID_BALL_STOP) {
		process_obstacle(ans, world, player, dst, dst, distance_keepout::friendly(player), 3 * POINTS_PER_OBSTACLE);
	}

	return ans;
}

bool AI::Nav::Util::valid_dst(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	return violation::get_violation_amount(dst, dst, world, player).violation_free();
}

bool AI::Nav::Util::valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	return violation::get_violation_amount(cur, dst, world, player).no_more_violating_than(violation::get_violation_amount(cur, cur, world, player));
}

bool AI::Nav::Util::valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int extra_flags) {
	return violation::get_violation_amount(cur, dst, world, player, extra_flags).no_more_violating_than(violation::get_violation_amount(cur, cur, world, player, extra_flags));
}


std::vector<Point> AI::Nav::Util::get_obstacle_boundaries(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	return get_obstacle_boundaries(world, player, 0);
}
#warning some magic numbers here need to clean up a bit
std::vector<Point> AI::Nav::Util::get_obstacle_boundaries(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int added_flags) {
	// this number must be >=3
	const int POINTS_PER_OBSTACLE = 6;
	std::vector<Point> ans;
	unsigned int flags = player->flags() | added_flags;
	const Field &f = world.field();

	if (flags & FLAG_AVOID_BALL_STOP) {
		process_obstacle(ans, world, player, world.ball().position(), world.ball().position(), distance_keepout::ball_stop(player), 3 * POINTS_PER_OBSTACLE);
	}

	if (flags & FLAG_STAY_OWN_HALF) {
		Point half_point1(0.0, -f.width() / 2);
		Point half_point2(0.0, f.width() / 2);
		process_obstacle(ans, world, player, half_point1, half_point2, distance_keepout::own_half(player), 7 * POINTS_PER_OBSTACLE);
	}

	if (flags & FLAG_AVOID_FRIENDLY_DEFENSE) {
		Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
		process_obstacle(ans, world, player, defense_point1, defense_point2, distance_keepout::friendly_defense(world, player), POINTS_PER_OBSTACLE);
	}

	if (flags & FLAG_AVOID_ENEMY_DEFENSE) {
		Point defense_point1(f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(f.length() / 2, f.defense_area_stretch() / 2);
		process_obstacle(ans, world, player, defense_point1, defense_point2, distance_keepout::enemy_defense(world, player), POINTS_PER_OBSTACLE);
	}

	if ((flags & FLAG_AVOID_BALL_TINY) && !(flags & FLAG_AVOID_BALL_STOP)) {
		process_obstacle(ans, world, player, world.ball().position(), world.ball().position(), distance_keepout::ball_tiny(player), POINTS_PER_OBSTACLE);
	}

	for (unsigned int i = 0; i < world.friendly_team().size(); i++) {
		AI::Nav::W::Player::Ptr rob = world.friendly_team().get(i);
		if (rob == player) {
			// points around self may help with trying to escape when stuck
			// that is why there are double the number of points here
			process_obstacle(ans, world, player, rob->position(), rob->position(), distance_keepout::friendly(player), 2 * POINTS_PER_OBSTACLE);
			continue;
		}
		process_obstacle(ans, world, player, rob->position(), rob->position(), distance_keepout::friendly(player), POINTS_PER_OBSTACLE);
	}

	for (unsigned int i = 0; i < world.enemy_team().size(); i++) {
		AI::Nav::W::Robot::Ptr rob = world.enemy_team().get(i);
		process_obstacle(ans, world, player, rob->position(), rob->position(), distance_keepout::enemy(world, player), POINTS_PER_OBSTACLE);
	}

	return ans;
}

#warning overlap method should choose whichever is written better
bool AI::Nav::Util::check_dest_valid(Point dest, World &world, Player::Ptr player) {
	double AVOID_TINY_CONSTANT = 0.15; // constant for AVOID_BALL_TINY
	double AVOID_PENALTY_CONSTANT = 0.40; // constant for AVOID_PENALTY_*
	const Field &field = world.field();
	double length, width;
	length = field.length();
	width = field.width();
	unsigned int flags = player->flags();
	// check for CLIP_PLAY_AREA simple rectangular bounds
	if ((flags & FLAG_CLIP_PLAY_AREA) == FLAG_CLIP_PLAY_AREA) {
		if (dest.x < -length / 2
		    || dest.x > length / 2
		    || dest.y < -width / 2
		    || dest.y > width / 2) {
			return false;
		}
	}
	// check for STAY_OWN_HALF
	if ((flags & FLAG_STAY_OWN_HALF) == FLAG_STAY_OWN_HALF) {
		if (dest.x > 0) {
			return false;
		}
	}
	/* field information for AVOID_*_DEFENSE_AREA checks
	 * defined by defense_area_radius distance from two goal posts
	 * and rectangular stretch directly in front of goal
	 */
	Point f_post1, f_post2, e_post1, e_post2;
	double defense_area_stretch, defense_area_radius;
	defense_area_stretch = field.defense_area_stretch();
	defense_area_radius = field.defense_area_radius();
	f_post1 = Point(-length / 2, defense_area_stretch / 2);
	f_post2 = Point(-length / 2, -defense_area_stretch / 2);
	// friendly case
	if ((flags & FLAG_AVOID_FRIENDLY_DEFENSE) == FLAG_AVOID_FRIENDLY_DEFENSE) {
		if (dest.x < -(length / 2) + defense_area_radius
		    && dest.x > -length / 2
		    && dest.y < defense_area_stretch / 2
		    && dest.y > -defense_area_stretch / 2) {
			return false;
		} else if ((dest - f_post1).len() < defense_area_radius
		           || (dest - f_post2).len() < defense_area_radius) {
			return false;
		}
	}
	// enemy goal posts
	e_post1 = Point(length / 2, defense_area_stretch / 2);
	e_post2 = Point(length / 2, -defense_area_stretch / 2);
	// enemy case
	if ((flags & FLAG_AVOID_ENEMY_DEFENSE) == FLAG_AVOID_ENEMY_DEFENSE) {
		if (dest.x > length / 2 - defense_area_radius - 0.20
		    && dest.x < length / 2
		    && dest.y < defense_area_stretch / 2
		    && dest.y > -defense_area_stretch / 2) {
			return false;
		} else if ((dest - e_post1).len() < defense_area_radius + 0.20 // 20cm from defense line
		           || (dest - e_post2).len() < defense_area_radius + 0.20) {
			return false;
		}
	}
	// ball checks (lol)
	const Ball &ball = world.ball();
	// AVOID_BALL_STOP
	if ((flags & FLAG_AVOID_BALL_STOP) == FLAG_AVOID_BALL_STOP) {
		if ((dest - ball.position()).len() < 0.5) {
			return false; // avoid ball by 50cm
		}
	}
	// AVOID_BALL_TINY
	if ((flags & FLAG_AVOID_BALL_TINY) == FLAG_AVOID_BALL_TINY) {
		if ((dest - ball.position()).len() < AVOID_TINY_CONSTANT) {
			return false; // avoid ball by this constant?
		}
	}
	/* PENALTY_KICK_* checks
	 * penalty marks are defined 450mm & equidistant from both goal posts,
	 * i assume that defense_area_stretch is a close enough approximation
	 */
	Point f_penalty_mark, e_penalty_mark;
	f_penalty_mark = Point((-length / 2) + defense_area_stretch, 0);
	e_penalty_mark = Point((length / 2) - defense_area_stretch, 0);
	if ((flags & FLAG_PENALTY_KICK_FRIENDLY) == FLAG_PENALTY_KICK_FRIENDLY) {
		if ((dest - f_penalty_mark).len() < AVOID_PENALTY_CONSTANT) {
			return false;
		}
	}
	if ((flags & FLAG_PENALTY_KICK_ENEMY) == FLAG_PENALTY_KICK_ENEMY) {
		if ((dest - e_penalty_mark).len() < AVOID_PENALTY_CONSTANT) {
			return false;
		}
	}
	/* check if dest is on another robot
	 * 2*robot radius (maybe we need some sort of margin?) from center to center
	 */
	FriendlyTeam &f_team = world.friendly_team();
	EnemyTeam &e_team = world.enemy_team();
	Player::Ptr f_player;
	Robot::Ptr e_player;
	// friendly case
	for (unsigned int i = 0; i < f_team.size(); i++) {
		f_player = f_team.get(i);
		if ((f_player != player) && ((dest - f_player->position()).len() < 2 * f_player->MAX_RADIUS)) {
			return false;
		}
	}
	// enemy case
	for (unsigned int i = 0; i < e_team.size(); i++) {
		e_player = e_team.get(i);
		if ((dest - e_player->position()).len() < 2 * e_player->MAX_RADIUS) {
			return false;
		}
	}

	return true;
}

