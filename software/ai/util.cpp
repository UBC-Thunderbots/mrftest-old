#include "ai/util.h"
#include "util/algorithm.h"
#include "geom/angle.h"
#include "geom/util.h"

#include <cmath>
#include <iostream>

using namespace AI;
using namespace Util;

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

	bool has_ball_vision(World &w, const Robot::Ptr p) {
		const Point dist = w.ball().position() - p->position();
		if (dist.len() > (Robot::MAX_RADIUS + Ball::RADIUS) * BALL_FRONT_FACTOR) return false;
		return angle_diff(dist.orientation(), p->orientation()) < degrees2radians(BALL_FRONT_ANGLE);
	}
}

#warning document this in ai/util.h
DoubleParam Util::DRIBBLE_TIMEOUT("if dribble > this time, force shoot (sec)", 2.0, 0.0, 20.0);

#warning document this in ai/util.h
DoubleParam Util::PLAYTYPE_WAIT_TIME("play: time we can get ready (sec)", 3.0, -1e99, 10.0);

//IntParam AGGRESIVENESS("aggresiveness", 10, 0, 10);

#warning document this in ai/util.h
DoubleParam Util::CHASE_BALL_DIST("chase: How close before chasing", Ball::RADIUS * 2, 0.0, Ball::RADIUS * 4);

#warning TODO: base this on distance
DoubleParam Util::ORI_CLOSE("kick: general accuracy (rads)", 5.0 * M_PI / 180.0, 0, M_PI / 2);

#warning see the Doxygen comment for this function: to start what?
bool Util::ball_close(World &w, const Robot::Ptr p) {
	const Point dist = w.ball().position() - p->position();
	return dist.len() < (Robot::MAX_RADIUS + Ball::RADIUS) * BALL_CLOSE_FACTOR;
}

bool Util::point_in_defense(World &w, const Point& pt) {
	const double defense_stretch = w.field().defense_area_stretch();
	const double defense_radius = w.field().defense_area_radius();
	const double field_length = w.field().length();
	const Point pole1 = Point(-field_length, defense_stretch/2 + defense_radius);
	const Point pole2 = Point(-field_length, -defense_stretch/2 - defense_radius);
	double dist1 = (pt-pole1).len();
	double dist2 = (pt-pole2).len();
	if( pt.x > -field_length/2 && pt.x < -field_length/2 + defense_radius && pt.y > -defense_stretch/2 && pt.y < defense_stretch/2 )
		return true;
	if( dist1 < defense_radius || dist2 < defense_radius )
		return true;
	return false;
}

bool Util::path_check(const Point& begin, const Point& end, const std::vector<Point>& obstacles, const double thresh) {
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

bool Util::path_check(const Point& begin, const Point& end, const std::vector<Robot::Ptr>& robots, const double thresh) {
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

#warning TODO: maybe the source to a point instead of defaulting to ball
bool Util::can_receive(World &w, const Player::Ptr passee) {
	const Ball &ball = w.ball();
	if ((ball.position() - passee->position()).lensq() < POS_CLOSE) {
		std::cerr << "can_pass: passe too close to ball" << std::endl;
		return true;
	}
	// if the passee is not facing the ball, forget it
	const Point ray = ball.position() - passee->position();
	if (angle_diff(ray.orientation(), passee->orientation()) > ORI_PASS_CLOSE) {
		// std::cout << "Util: angle diff = " << angle_diff(ray.orientation(), passee->orientation()) << std::endl; 
		return false;
	}
	// if(!path_check(ball->position(), passee->position(), w.enemy.get_robots(), SHOOT_ALLOWANCE + Robot::MAX_RADIUS + Ball::RADIUS)) return false;
	const Point direction = ray.norm();
	const double distance = (ball.position() - passee->position()).len();
	for (size_t i = 0; i < w.enemy.size(); ++i) {
		const Robot::Ptr rob = w.enemy.get_robot(i);
		const Point rp = rob->position() - passee->position();
		const double proj = rp.dot(direction);
		const double perp = sqrt(rp.dot(rp) - proj * proj);
		if (proj <= 0) continue;
		if (proj < distance && perp < SHOOT_ALLOWANCE + Robot::MAX_RADIUS + Ball::RADIUS) {
			return false;
		}
	}
	for (size_t i = 0; i < w.friendly.size(); ++i) {
		const Player::Ptr plr = w.friendly.get_player(i);
		if (posses_ball(w, plr) || plr == passee) continue;
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

std::pair<Point, double> Util::calc_best_shot(const Field& f, const std::vector<Point>& obstacles, const Point& p, const double radius) {
	const Point p1 = Point(f.length()/2.0,-f.goal_width()/2.0);
	const Point p2 = Point(f.length()/2.0,f.goal_width()/2.0);
	return angle_sweep_circles(p, p1, p2, obstacles, radius);
}

std::pair<Point, double> Util::calc_best_shot(const World &w, const Player::Ptr pl, const bool consider_friendly) {
	std::vector<Point> obstacles;
	const EnemyTeam &enemy(w.enemy);
	for (size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy[i]->position());
	}
	if (consider_friendly) {
		const FriendlyTeam &friendly(w.friendly);
		for (size_t i = 0; i < friendly.size(); ++i) {
			const Player::Ptr fpl = friendly[i];
			if (fpl == pl) continue;
			obstacles.push_back(fpl->position());
		}
	}
	std::pair<Point, double> best_shot = calc_best_shot(w.field(), obstacles, pl->position());
	return best_shot;
}

double Util::calc_goal_visibility_angle(const World &w, const Player::Ptr pl, const bool consider_friendly) {
	return calc_best_shot(w, pl, consider_friendly).second;
}

std::vector<Player::Ptr> Util::get_friends(const FriendlyTeam& friendly, const std::vector<Player::Ptr>& exclude) {
	std::vector<Player::Ptr> friends;
	for (size_t i = 0; i < friendly.size(); ++i) {
		const Player::Ptr plr(friendly.get_player(i));
		if (exists(exclude.begin(), exclude.end(), plr)) continue;
		friends.push_back(plr);
	}
	return friends;
}

#warning document the meaning of the return value of this (is it an index into the vector?)
int Util::choose_best_pass(World &w, const std::vector<Player::Ptr>& friends) {
	double bestangle = 0;
	double bestdist = 1e99;
	int bestidx = -1;
	for (size_t i = 0; i < friends.size(); ++i) {
		// see if this player is on line of sight
		if (!Util::can_receive(w, friends[i])) continue;
		// choose the most favourable distance
		const double dist = (friends[i]->position() - w.ball().position()).len();
		const double angle = calc_goal_visibility_angle(w, friends[i]);
		if (bestidx == -1 || angle > bestangle || (angle == bestangle && dist < bestdist)) {
			bestangle = angle;
			bestdist = dist;
			bestidx = i;
		}
	}
	return bestidx;
}

bool Util::has_ball(World &w, const Player::Ptr p) {
	// return p->sense_ball() >= HAS_BALL_TIME || (HAS_BALL_USE_VISION && has_ball_vision(w, p));
	if (HAS_BALL_USE_VISION) {
		return p->sense_ball() >= HAS_BALL_TIME || has_ball_vision(w, p);
	} else {
		return p->sense_ball() >= HAS_BALL_TIME;
	}
}

bool Util::posses_ball(World &w, const Player::Ptr p) {
	if (POSSES_BALL_IS_HAS_BALL) return has_ball(w, p);
	return has_ball(w, p) || ball_close(w, p);
}

bool Util::friendly_has_ball(World &w) {
	const FriendlyTeam& friendly = w.friendly;
	for (size_t i = 0; i < friendly.size(); ++i) {
		if (has_ball(w, friendly[i])) return true;
	}
	return false;
}

/*
bool Util::posses_ball(World &w, const Robot::Ptr r) {
	return ball_close(w, r);
}

bool Util::enemy_posses_ball(World &w) {
	const EnemyTeam& enemy = w.enemy;
	for (size_t i = 0; i < enemy.size(); ++i) {
		if (posses_ball(w, enemy[i])) return true;
	}
	return false;
}
*/

bool Util::friendly_posses_ball(World &w) {
	switch(w.playtype()){
		case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
		case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
		case PlayType::PREPARE_KICKOFF_ENEMY:
		case PlayType::EXECUTE_KICKOFF_ENEMY:
		case PlayType::PREPARE_PENALTY_ENEMY:
		case PlayType::EXECUTE_PENALTY_ENEMY:
			return false;
			break;
		default:
			break;
	}
	const FriendlyTeam& friendly = w.friendly;
	for (size_t i = 0; i < friendly.size(); ++i) {
		if (posses_ball(w, friendly[i])) return true;
	}
	return false;
}

int Util::calc_baller(World &w, const std::vector<Player::Ptr>& players) {
	for (size_t i = 0; i < players.size(); ++i) {
		if (posses_ball(w, players[i])) return static_cast<int>(i);
	}
	return -1;
}

