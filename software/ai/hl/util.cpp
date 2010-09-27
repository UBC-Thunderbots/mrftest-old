#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include <cmath>
#include <iostream>

using AI::HL::W::World;
using AI::HL::W::Player;
using AI::HL::W::Robot;
using AI::HL::W::Ball;
using AI::HL::W::Field;
using AI::HL::W::EnemyTeam;
using AI::HL::W::FriendlyTeam;

namespace {

#warning Magic constant
	const double SHOOT_ALLOWANCE = Ball::RADIUS;

	const double EPS = 1e-9;

	BoolParam HAS_BALL_USE_VISION("has_ball: use vision", false);
	BoolParam POSSES_BALL_IS_HAS_BALL("posses_ball: is has ball", false);
	IntParam HAS_BALL_TIME("has_ball: # of sense ball for to be true", 2, 1, 10);
	DoubleParam BALL_CLOSE_FACTOR("ball_close Distance Factor", 1.1, 1.0, 1.5);
	DoubleParam BALL_FRONT_ANGLE("has_ball_vision: angle (degrees)", 20.0, 0.0, 60.0);
	DoubleParam BALL_FRONT_FACTOR("has_ball_vision: dist factor", 0.9, 0.1, 2.0);
}

DoubleParam AI::HL::Util::DRIBBLE_TIMEOUT("if dribble > this time, force shoot (sec)", 2.0, 0.0, 20.0);

DoubleParam AI::HL::Util::PLAYTYPE_WAIT_TIME("play: time we can get ready (sec)", 3.0, -1e99, 10.0);

DoubleParam AI::HL::Util::CHASE_BALL_DIST("chase: How close before chasing", Ball::RADIUS * 2, 0.0, Ball::RADIUS * 4);

#warning TODO: base this on distance.
DoubleParam AI::HL::Util::ORI_CLOSE("kick: general accuracy (rads)", 5.0 * M_PI / 180.0, 0, M_PI / 2);

const double AI::HL::Util::POS_CLOSE = AI::HL::W::Robot::MAX_RADIUS / 4.0;

const double AI::HL::Util::POS_EPS = 1e-12;

const double AI::HL::Util::VEL_CLOSE = 1e-2;

const double AI::HL::Util::VEL_EPS = 1e-12;

const double AI::HL::Util::ORI_PASS_CLOSE = 45.0 / 180.0 * M_PI;

const double AI::HL::Util::HAS_BALL_ALLOWANCE = 3.0;

const double AI::HL::Util::HAS_BALL_TIME = 2.0 / 15.0;

bool AI::HL::Util::path_check(const Point& begin, const Point& end, const std::vector<Point>& obstacles, const double thresh) {
	const Point direction = (end - begin).norm();
	const double dist = (end - begin).len();
	for (size_t i = 0; i < obstacles.size(); ++i) {
		const Point ray = obstacles[i] - begin;
		const double proj = ray.dot(direction);
		const double perp = fabs(ray.cross(direction));
		if (proj <= 0) continue;
		if (proj < dist && perp < thresh)
			return false;
	}
	return true;
}

#warning TODO: add more features to this function
bool AI::HL::Util::path_check(const Point& begin, const Point& end, const std::vector<Robot::Ptr>& robots, const double thresh) {
	const Point direction = (end - begin).norm();
	const double dist = (end - begin).len();
	for (size_t i = 0; i < robots.size(); ++i) {
		const Point ray = robots[i]->position() - begin;
		const double proj = ray.dot(direction);
		const double perp = fabs(ray.cross(direction));
		if (proj <= 0) continue;
		if (proj < dist && perp < thresh)
			return false;
	}
	return true;
}

#warning TODO: maybe the source to a point instead of defaulting to ball.
bool AI::HL::Util::can_receive(World& world, const Player::Ptr passee) {
	const Ball& ball = world.ball();
	if ((ball.position() - passee->position()).lensq() < POS_CLOSE) {
		std::cerr << "can_pass: passe too close to ball" << std::endl;
		return true;
	}
	// if the passee is not facing the ball, forget it
	const Point ray = ball.position() - passee->position();
	if (angle_diff(ray.orientation(), passee->orientation()) > ORI_PASS_CLOSE) {
		return false;
	}

	const Point direction = ray.norm();
	const double distance = (ball.position() - passee->position()).len();
	EnemyTeam& enemy = world.enemy_team();
	for (size_t i = 0; i < enemy.size(); ++i) {
		const Robot::Ptr rob = enemy.get(i);
		const Point rp = rob->position() - passee->position();
		const double proj = rp.dot(direction);
		const double perp = sqrt(rp.dot(rp) - proj * proj);
		if (proj <= 0) continue;
		if (proj < distance && perp < SHOOT_ALLOWANCE + Robot::MAX_RADIUS + Ball::RADIUS) {
			return false;
		}
	}
	FriendlyTeam& friendly = world.friendly_team();
	for (size_t i = 0; i < friendly.size(); ++i) {
		const Player::Ptr plr = friendly.get(i);
		if (posses_ball(world, plr) || plr == passee) continue;
		const Point rp = plr->position() - passee->position();
		const double proj = rp.dot(direction);
		const double perp = sqrt(rp.dot(rp) - proj * proj);
		if (proj <= 0) continue;
		if (proj < distance && perp < SHOOT_ALLOWANCE + Robot::MAX_RADIUS + Ball::RADIUS) {
			return false;
		}
	}
	return true;
}

std::pair<Point, double> AI::HL::Util::calc_best_shot(const Field& f, const std::vector<Point>& obstacles, const Point& p, const double radius) {
	const Point p1 = Point(f.length()/2.0, -f.goal_width()/2.0);
	const Point p2 = Point(f.length()/2.0, f.goal_width()/2.0);
	return angle_sweep_circles(p, p1, p2, obstacles, radius);
}

std::pair<Point, double> AI::HL::Util::calc_best_shot(World& world, const Player::Ptr p) {
	std::vector<Point> obstacles;
	EnemyTeam& enemy = world.enemy_team();
	for (size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}
	FriendlyTeam& friendly = world.friendly_team();
	for (size_t i = 0; i < friendly.size(); ++i) {
		const Player::Ptr fpl = friendly.get(i);
		if (fpl == p) continue;
		obstacles.push_back(fpl->position());
	}
	return calc_best_shot(world.field(), obstacles, p->position(), Robot::MAX_RADIUS);
}

bool AI::HL::Util::ball_close(World& world, const Robot::Ptr robot) {
	const Point dist = world.ball().position() - robot->position();
	return dist.len() < (Robot::MAX_RADIUS + Ball::RADIUS) * BALL_CLOSE_FACTOR;
}

bool AI::HL::Util::posses_ball(World &world, const Player::Ptr player) {
	if (player->has_ball()) return true;
	if (POSSES_BALL_IS_HAS_BALL) return false;
	return ball_close(world, player);
}

bool AI::HL::Util::posses_ball(World &world, const Robot::Ptr robot) {
	return ball_close(world, robot);
}

Player::Ptr AI::HL::Util::calc_baller(World &world, const std::vector<Player::Ptr>& players) {
	for (size_t i = 0; i < players.size(); ++i) {
		if (posses_ball(world, players[i])) return players[i];
	}
	return Player::Ptr();
}

