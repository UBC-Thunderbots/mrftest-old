#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>

using namespace AI::HL::W;

const double AI::HL::Util::POS_CLOSE = AI::HL::W::Robot::MAX_RADIUS / 4.0;

const double AI::HL::Util::POS_EPS = 1e-12;

const double AI::HL::Util::VEL_CLOSE = 1e-2;

const double AI::HL::Util::VEL_EPS = 1e-12;

bool AI::HL::Util::point_in_friendly_defense(const Field &field, const Point p) {
	const double defense_stretch = field.defense_area_stretch();
	const double defense_radius = field.defense_area_radius();
	const Point friendly_goal = field.friendly_goal();
	const Point pole1 = Point(friendly_goal.x, defense_stretch / 2);
	const Point pole2 = Point(friendly_goal.x, -defense_stretch / 2);
	double dist1 = (p - pole1).len();
	double dist2 = (p - pole2).len();
	if (p.x > friendly_goal.x && p.x < friendly_goal.x + defense_radius && p.y > -defense_stretch / 2 && p.y < defense_stretch / 2) {
		return true;
	}
	if (dist1 < defense_radius || dist2 < defense_radius) {
		return true;
	}
	return false;
}

Point AI::HL::Util::crop_point_to_field(const Field &field, const Point p) {
	double x = p.x;
	double y = p.y;
	if (p.x > field.length() / 2) {
		x = field.length() / 2;
	}
	if (p.x < -(field.length() / 2)) {
		x = -(field.length() / 2);
	}
	if (p.y > field.width() / 2) {
		y = field.width() / 2;
	}
	if (p.y < -(field.width() / 2)) {
		y = -(field.width() / 2);
	}

	return Point(x, y);
}

bool AI::HL::Util::path_check(const Point &begin, const Point &end, const std::vector<Point> &obstacles, const double thresh) {
	const Point direction = (end - begin).norm();
	const double dist = (end - begin).len();
	for (Point i : obstacles) {
		const Point ray = i - begin;
		const double proj = ray.dot(direction);
		const double perp = fabs(ray.cross(direction));
		if (proj <= 0) {
			continue;
		}
		if (proj < dist && perp < thresh) {
			return false;
		}
	}
	return true;
}

#warning TODO: add more features to this function
bool AI::HL::Util::path_check(const Point &begin, const Point &end, const std::vector<Robot> &robots, const double thresh) {
	const Point direction = (end - begin).norm();
	const double dist = (end - begin).len();
	for (const Robot &i : robots) {
		const Point ray = i.position() - begin;
		const double proj = ray.dot(direction);
		const double perp = fabs(ray.cross(direction));
		if (proj <= 0) {
			continue;
		}
		if (proj < dist && perp < thresh) {
			return false;
		}
	}
	return true;
}

std::pair<Point, Angle> AI::HL::Util::calc_best_shot(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius) {
	const Point p1 = Point(f.length() / 2.0, -f.goal_width() / 2.0);
	const Point p2 = Point(f.length() / 2.0, f.goal_width() / 2.0);
	return angle_sweep_circles(p, p1, p2, obstacles, radius * Robot::MAX_RADIUS);
}

std::vector<std::pair<Point, Angle> > AI::HL::Util::calc_best_shot_all(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius) {
	const Point p1 = Point(f.length() / 2.0, -f.goal_width() / 2.0);
	const Point p2 = Point(f.length() / 2.0, f.goal_width() / 2.0);
	return angle_sweep_circles_all(p, p1, p2, obstacles, radius * Robot::MAX_RADIUS);
}

std::pair<Point, Angle> AI::HL::Util::calc_best_shot(World world, const Player player, const double radius) {
	std::vector<Point> obstacles;
	EnemyTeam enemy = world.enemy_team();
	FriendlyTeam friendly = world.friendly_team();
	obstacles.reserve(enemy.size() + friendly.size());
	for (const Robot i : enemy) {
		obstacles.push_back(i.position());
	}
	for (const Player fpl : friendly) {
		if (fpl == player) {
			continue;
		}
		obstacles.push_back(fpl.position());
	}
	std::pair<Point, Angle> best_shot = calc_best_shot(world.field(), obstacles, player.position(), radius);
	// if there is no good shot at least make the
	// target within the goal area
	if (best_shot.second <= Angle::zero()) {
		Point temp = Point(world.field().length() / 2.0, 0.0);
		best_shot.first = temp;
	}
	return best_shot;
}

std::vector<std::pair<Point, Angle> > AI::HL::Util::calc_best_shot_all(World world, const Player player, const double radius) {
	std::vector<Point> obstacles;
	EnemyTeam enemy = world.enemy_team();
	FriendlyTeam friendly = world.friendly_team();
	obstacles.reserve(enemy.size() + friendly.size());
	for (const Robot i : enemy) {
		obstacles.push_back(i.position());
	}
	for (const Player fpl : friendly) {
		if (fpl == player) {
			continue;
		}
		obstacles.push_back(fpl.position());
	}
	return calc_best_shot_all(world.field(), obstacles, player.position(), radius);
}

std::vector<Player> AI::HL::Util::get_players(FriendlyTeam friendly) {
	return std::vector<Player>(friendly.begin(), friendly.end());
}

std::vector<Robot> AI::HL::Util::get_robots(EnemyTeam enemy) {
	return std::vector<Robot>(enemy.begin(), enemy.end());
}

