#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>

using namespace AI::HL::W;

namespace {
	BoolParam posses_ball_is_has_ball("posses ball is has ball", false);
	DoubleParam ball_close_factor("distance for ball possesion (x ball radius)", 2.0, 1.0, 3.0);
}

#warning hardware depending parameters should move somewhere else
DoubleParam AI::HL::Util::shoot_accuracy("Shooting Accuracy General (degrees)", 5.0, 0.1, 10.0);

DoubleParam AI::HL::Util::dribble_timeout("if dribble > this time, force shoot (sec)", 2.0, 0.0, 20.0);

DoubleParam AI::HL::Util::get_ready_time("time we can prepare during special plays (sec)", 3.0, -1e99, 10.0);

const double AI::HL::Util::POS_CLOSE = AI::HL::W::Robot::MAX_RADIUS / 4.0;

const double AI::HL::Util::POS_EPS = 1e-12;

const double AI::HL::Util::VEL_CLOSE = 1e-2;

const double AI::HL::Util::VEL_EPS = 1e-12;

const double AI::HL::Util::ORI_PASS_CLOSE = 45.0 / 180.0 * M_PI;

const double AI::HL::Util::HAS_BALL_ALLOWANCE = 3.0;

const double AI::HL::Util::HAS_BALL_TIME = 2.0 / 15.0;

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

bool AI::HL::Util::path_check(const Point &begin, const Point &end, const std::vector<Point> &obstacles, const double thresh) {
	const Point direction = (end - begin).norm();
	const double dist = (end - begin).len();
	for (std::size_t i = 0; i < obstacles.size(); ++i) {
		const Point ray = obstacles[i] - begin;
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
bool AI::HL::Util::path_check(const Point &begin, const Point &end, const std::vector<Robot::Ptr> &robots, const double thresh) {
	const Point direction = (end - begin).norm();
	const double dist = (end - begin).len();
	for (std::size_t i = 0; i < robots.size(); ++i) {
		const Point ray = robots[i]->position() - begin;
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

#warning TODO: maybe the source to a point instead of defaulting to ball.
bool AI::HL::Util::can_receive(World &world, const Player::Ptr passee) {
	const Ball &ball = world.ball();
	if ((ball.position() - passee->position()).lensq() < POS_CLOSE) {
		LOG_ERROR("can_pass: passee too close to ball");
		return true;
	}
	// if the passee is not facing the ball, forget it
	const Point ray = ball.position() - passee->position();
	if (angle_diff(ray.orientation(), passee->orientation()) > ORI_PASS_CLOSE) {
		return false;
	}

	const Point direction = ray.norm();
	const double distance = (ball.position() - passee->position()).len();
	EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		const Robot::Ptr rob = enemy.get(i);
		const Point rp = rob->position() - passee->position();
		const double proj = rp.dot(direction);
		const double perp = sqrt(rp.dot(rp) - proj * proj);
		if (proj <= 0) {
			continue;
		}
		if (proj < distance && perp < shoot_accuracy + Robot::MAX_RADIUS + Ball::RADIUS) {
			return false;
		}
	}
	FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::Ptr plr = friendly.get(i);
		if (posses_ball(world, plr) || plr == passee) {
			continue;
		}
		const Point rp = plr->position() - passee->position();
		const double proj = rp.dot(direction);
		const double perp = sqrt(rp.dot(rp) - proj * proj);
		if (proj <= 0) {
			continue;
		}
		if (proj < distance && perp < shoot_accuracy + Robot::MAX_RADIUS + Ball::RADIUS) {
			return false;
		}
	}
	return true;
}

std::pair<Point, double> AI::HL::Util::calc_best_shot(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius) {
	const Point p1 = Point(f.length() / 2.0, -f.goal_width() / 2.0);
	const Point p2 = Point(f.length() / 2.0, f.goal_width() / 2.0);
	return angle_sweep_circles(p, p1, p2, obstacles, radius * Robot::MAX_RADIUS);
}

std::pair<Point, double> AI::HL::Util::calc_best_shot(World &world, const Player::Ptr player, const double radius) {
	std::vector<Point> obstacles;
	EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}
	FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::Ptr fpl = friendly.get(i);
		if (fpl == player) {
			continue;
		}
		obstacles.push_back(fpl->position());
	}
	return calc_best_shot(world.field(), obstacles, player->position(), radius);
}

bool AI::HL::Util::ball_close(World &world, const Robot::Ptr robot) {
	const Point dist = world.ball().position() - robot->position();
	return dist.len() < (Robot::MAX_RADIUS + Ball::RADIUS * ball_close_factor);
}

bool AI::HL::Util::posses_ball(World &world, const Player::Ptr player) {
	if (player->has_ball()) {
		return true;
	}
	if (posses_ball_is_has_ball) {
		return false;
	}
	return ball_close(world, player);
}

bool AI::HL::Util::posses_ball(World &world, const Robot::Ptr robot) {
	return ball_close(world, robot);
}

Player::Ptr AI::HL::Util::calc_baller(World &world, const std::vector<Player::Ptr> &players) {
	for (std::size_t i = 0; i < players.size(); ++i) {
		if (posses_ball(world, players[i])) {
			return players[i];
		}
	}
	return Player::Ptr();
}

std::vector<Player::Ptr> AI::HL::Util::get_players(FriendlyTeam &friendly) {
	std::vector<Player::Ptr> players;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		players.push_back(friendly.get(i));
	}
	return players;
}

std::vector<Player::Ptr> AI::HL::Util::get_players_exclude(FriendlyTeam &friendly, std::vector<Player::Ptr>& others) {
	std::vector<Player::Ptr> players;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (!exists(others.begin(), others.end(), friendly.get(i))) {
			players.push_back(friendly.get(i));
		}
	}
	return players;
}

std::vector<Robot::Ptr> AI::HL::Util::get_robots(EnemyTeam &enemy) {
	std::vector<Robot::Ptr> robots;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		robots.push_back(enemy.get(i));
	}
	return robots;
}

void AI::HL::Util::waypoints_matching(const std::vector<Player::Ptr> &players, std::vector<Point> &waypoints) {
	// TODO: make more efficient
	std::vector<Point> locations;
	for (std::size_t i = 0; i < players.size(); ++i) {
		locations.push_back(players[i]->position());
	}
	const std::vector<Point> waypoints_orig = waypoints;
	std::vector<std::size_t> order = dist_matching(locations, waypoints);
	for (std::size_t i = 0; i < waypoints.size(); ++i) {
		waypoints[i] = waypoints_orig[order[i]];
	}
}

Player::Ptr AI::HL::Util::choose_best_pass(World &world, const std::vector<Player::Ptr> &friends) {
	double bestangle = 0;
	double bestdist = 1e99;
	Player::Ptr passee;
	for (size_t i = 0; i < friends.size(); ++i) {
		// see if this player is on line of sight
		if (!AI::HL::Util::can_receive(world, friends[i])) {
			continue;
		}
		// choose the most favourable distance
		const double dist = (friends[i]->position() - world.ball().position()).len();
		const double angle = AI::HL::Util::calc_best_shot(world, friends[i]).second;
		if (!passee.is() || angle > bestangle || (angle == bestangle && dist < bestdist)) {
			bestangle = angle;
			bestdist = dist;
			passee = friends[i];
		}
	}
	return passee;
}

