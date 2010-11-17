#include "ai/navigator/util.h"
#include "ai/flags.h"
#include <algorithm>
#include <vector>

using namespace AI::Flags;
using namespace AI::Nav::W;

namespace {
	const double EPS = 1e-9;
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

	struct violation {
		double enemy;
		double friendly;
		double play_area;
		double ball_stop;
		double ball_tiny;
		double friendly_defense;
		double enemy_defense;
		double own_half;
		double penalty_kick_friendly;
		double penalty_kick_enemy;

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

	// should be still work when cur == dst
	double get_enemy_violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double player_rad = player->MAX_RADIUS;
		double violate = 0.0;
		// avoid enemy robots
		for (unsigned int i = 0; i < world.enemy_team().size(); i++) {
			AI::Nav::W::Robot::Ptr rob = world.enemy_team().get(i);
			double enemy_rad = rob->MAX_RADIUS;
			double circle_radius = player_rad + enemy_rad + ENEMY_BUFFER;
			double dist = lineseg_point_dist(rob->position(), cur, dst);
			violate = std::max(violate, circle_radius - dist);
		}
		return violate;
	}

	// should be still work when cur == dst
	double get_friendly_violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
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

	double get_ball_stop_violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double player_rad = player->MAX_RADIUS;
		double violate = 0.0;
		const Ball &ball = world.ball();
		double circle_radius = Ball::RADIUS + player_rad + BALL_STOP_BUFFER;
		double dist = lineseg_point_dist(ball.position(), cur, dst);
		violate = std::max(violate, circle_radius - dist);
		return violate;
	}

	double get_defense_area_violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double violate = 0.0;
		const Field &f = world.field();
		double defense_dist = f.defense_area_radius() + player->MAX_RADIUS + DEFENSE_AREA_BUFFER;

		Point zero;
		Rect r(zero, defense_dist, f.defense_area_stretch());
		Point trans_defense_area(-(f.length() / 2 - r.width()), -r.height() / 2);
		r.translate(trans_defense_area);

		Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
		double dist = std::min(lineseg_point_dist(defense_point1, cur, dst), lineseg_point_dist(defense_point1, cur, dst));
		violate = std::max(violate, defense_dist - dist);
		std::vector<Point> p = line_rect_intersect(r, cur, dst);
		for (std::vector<Point>::const_iterator it = p.begin(); it != p.end(); it++) {
			double x = it->x + f.length() / 2;
			violate = std::max(violate, defense_dist - x);
		}
		return violate;
	}

	double get_offense_area_violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double violate = 0.0;
		const Field &f = world.field();
		double defense_dist = f.defense_area_radius() + player->MAX_RADIUS + DEFENSE_AREA_BUFFER;
		Point zero;
		Rect r(zero, defense_dist, f.defense_area_stretch());
		Point trans_defense_area(f.length() / 2, -r.height() / 2);
		r.translate(trans_defense_area);

		Point defense_point1(f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(f.length() / 2, f.defense_area_stretch() / 2);
		double dist = std::min(lineseg_point_dist(defense_point1, cur, dst), lineseg_point_dist(defense_point1, cur, dst));
		violate = std::max(violate, defense_dist - dist);
		std::vector<Point> p = line_rect_intersect(r, cur, dst);
		for (std::vector<Point>::const_iterator it = p.begin(); it != p.end(); it++) {
			double x = it->x - f.length() / 2;
			violate = std::max(violate, defense_dist + x);
		}
		return violate;
	}

	double get_ball_tiny_violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double player_rad = player->MAX_RADIUS;
		double violate = 0.0;
		const Ball &ball = world.ball();
		double circle_radius = Ball::RADIUS + player_rad + BALL_TINY_BUFFER;
		double dist = lineseg_point_dist(ball.position(), cur, dst);
		violate = std::max(violate, circle_radius - dist);
		return violate;
	}

	violation get_violation_amount(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		violation v;
		v.friendly = get_friendly_violation(cur, dst, world, player);
		v.enemy = get_enemy_violation(cur, dst, world, player);
		unsigned int flags = player->flags();
		if (flags & FLAG_CLIP_PLAY_AREA) {
		}
		if (flags & FLAG_AVOID_BALL_STOP) {
			v.ball_stop = get_ball_stop_violation(cur, dst, world, player);
		}
		if (flags & FLAG_AVOID_BALL_TINY) {
			v.ball_tiny = get_ball_tiny_violation(cur, dst, world, player);
		}
		if (flags & FLAG_AVOID_FRIENDLY_DEFENSE) {
			v.friendly_defense = get_defense_area_violation(cur, dst, world, player);
		}
		if (flags & FLAG_AVOID_ENEMY_DEFENSE) {
			v.friendly_defense = get_offense_area_violation(cur, dst, world, player);
		}
		return v;
	}
};

bool AI::Nav::Util::valid_dst(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	return get_violation_amount(dst, dst, world, player).violation_free();
}

bool AI::Nav::Util::valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	violation a = get_violation_amount(cur, cur, world, player);
	violation b = get_violation_amount(cur, dst, world, player);
	return b.no_more_violating_than(a);
}

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

