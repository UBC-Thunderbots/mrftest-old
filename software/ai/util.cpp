#include "ai/util.h"
#include "util/algorithm.h"
#include "geom/angle.h"
#include "geom/util.h"
#include <cmath>

#include <iostream>

namespace {

#warning Magic constant
	const double SHOOT_ALLOWANCE = ball::RADIUS;

	const double BALL_FRONT_ANGLE = M_PI / 6;

	const double EPS = 1e-9;
}

namespace ai_util {

	/*
	const std::vector<point> calc_candidates(const world::ptr world) {
		std::vector<point> candidates(SHOOTING_SAMPLE_POINTS);
		const field &the_field(world->field());

		const double goal_width = the_field.goal_width() - 2*robot::MAX_RADIUS;
		const double delta = goal_width / SHOOTING_SAMPLE_POINTS;

		for (size_t i = 0; i < SHOOTING_SAMPLE_POINTS; ++i) {
			point p(the_field.length()/2.0, -the_field.goal_width()/2.0 + robot::MAX_RADIUS + i * delta);
			candidates[i] = p;
		}
		return candidates;
	}
	*/

	bool ball_close(const world::ptr w, const robot::ptr p) {
		const point dist = w->ball()->position() - p->position();
		if (dist.len() > robot::MAX_RADIUS + ball::RADIUS * 2) return false;
		return angle_diff(dist.orientation(), p->orientation()) < M_PI / 2;
	}
	
	bool ball_front(const world::ptr w, const robot::ptr p) {
		const point dist = w->ball()->position() - p->position();
		if (dist.len() > robot::MAX_RADIUS + ball::RADIUS * 2) return false;
		return angle_diff(dist.orientation(), p->orientation()) < BALL_FRONT_ANGLE;
	}

	bool point_in_defense(const world::ptr w, const point& pt) {
		const double defense_stretch = w->field().defense_area_stretch();
		const double defense_radius = w->field().defense_area_radius();
		const double field_length = w->field().length();
		const point pole1 = point(-field_length, defense_stretch/2 + defense_radius);
		const point pole2 = point(-field_length, -defense_stretch/2 - defense_radius);
		double dist1 = (pt-pole1).len();
		double dist2 = (pt-pole2).len();
		if( pt.x > -field_length/2 && pt.x < -field_length/2 + defense_radius && pt.y > -defense_stretch/2 && pt.y < defense_stretch/2 )
			return true;
		if( dist1 < defense_radius || dist2 < defense_radius )
			return true;
		return false;
	}

	bool path_check(const point& begin, const point& end, const std::vector<point>& obstacles, const double thresh) {
		const point direction = (end - begin).norm();
		const double dist = (end - begin).len();
		for (size_t i = 0; i < obstacles.size(); ++i) {
			const point ray = obstacles[i] - begin;
			const double proj = ray.dot(direction);
			const double perp = fabs(ray.cross(direction));
			if (proj <= 0) continue;
			if (proj < dist && perp < thresh)
				return false;
		}
		return true;
	}

	bool path_check(const point& begin, const point& end, const std::vector<robot::ptr>& robots, const double thresh) {
		const point direction = (end - begin).norm();
		const double dist = (end - begin).len();
		for (size_t i = 0; i < robots.size(); ++i) {
			const point ray = robots[i]->position() - begin;
			const double proj = ray.dot(direction);
			const double perp = fabs(ray.cross(direction));
			if (proj <= 0) continue;
			if (proj < dist && perp < thresh)
				return false;
		}
		return true;
	}

	bool can_receive(const world::ptr w, const player::ptr passee) {
		const ball::ptr ball = w->ball();
		if ((ball->position() - passee->position()).lensq() < POS_CLOSE) {
			std::cerr << "can_pass: passe too close to ball" << std::endl;
			return true;
		}
		// if the passee is not facing the ball, forget it
		const point ray = ball->position() - passee->position();
		if (angle_diff(ray.orientation(), passee->orientation()) > ORI_PASS_CLOSE) {
			// std::cout << "ai_util: angle diff = " << angle_diff(ray.orientation(), passee->orientation()) << std::endl; 
			return false;
		}
		// if(!path_check(ball->position(), passee->position(), w->enemy.get_robots(), SHOOT_ALLOWANCE + robot::MAX_RADIUS + ball::RADIUS)) return false;
		const point direction = ray.norm();
		const double distance = (ball->position() - passee->position()).len();
		for (size_t i = 0; i < w->enemy.size(); ++i) {
			const robot::ptr rob = w->enemy.get_robot(i);
			const point rp = rob->position() - passee->position();
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);
			if (proj <= 0) continue;
			if (proj < distance && perp < SHOOT_ALLOWANCE + robot::MAX_RADIUS + ball::RADIUS) {
				return false;
			}
		}
		for (size_t i = 0; i < w->friendly.size(); ++i) {
			const player::ptr plr = w->friendly.get_player(i);
			if (posses_ball(w, plr) || plr == passee) continue;
			const point rp = plr->position() - passee->position();
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);
			if (proj <= 0) continue;
			if (proj < distance && perp < SHOOT_ALLOWANCE + robot::MAX_RADIUS + ball::RADIUS) {
				return false;
			}
		}
		return true;
	}

	// depecreciated
	/*
	int calc_best_shot_old(const player::ptr player, const world::ptr world) {
		std::vector<point> candidates = calc_candidates(world);
		int best_point = -1;
		double best_score = -1;

		const team &opponent_team(world->enemy);
		double proximity, score, dist;

		for (unsigned int i = 0; i < SHOOTING_SAMPLE_POINTS; ++i) {		
			point projection = candidates[i] - player->position();
			score = 0;
			for (unsigned int j = 0; j < opponent_team.size(); ++j) {
				point other = opponent_team.get_robot(j)->position() - player->position();
				proximity = (other).dot(projection.norm());
				// don't process the robot if it's behind the shooter
				if (proximity >= robot::MAX_RADIUS) {
					// calculate how close the opponent robot is to our robot in proportion to our projection, 0 if the opponent robot is
					// at our robot, 1 if the opponent robot is at the target.			
					// scale_factor = proximity / projection.len();

					dist = sqrt(other.lensq() - proximity * proximity);

					if (dist <= robot::MAX_RADIUS) {
						break;
					}	
					// use a 1/dist function to determine to score: the closer the opponent robot is to the projection, the higher the score
					score += 1.0 / dist;
				}
			}
			if (best_point == -1 || score < best_score 
					|| (score == best_score && abs(2*i+1-candidates.size()) < abs(2*best_score+1-candidates.size()))) {
				best_point = i;
				best_score = score;
			}
		}
		return best_point;
	}
	*/

	std::pair<point, double> calc_best_shot(const field& f, const std::vector<point>& obstacles, const point& p) {
		const point p1 = point(f.length()/2.0,-f.goal_width()/2.0);
		const point p2 = point(f.length()/2.0,f.goal_width()/2.0);
		return angle_sweep_circles(p, p1, p2, obstacles, robot::MAX_RADIUS);
	}

	std::pair<point, double> calc_best_shot(const world::ptr w, const player::ptr pl, const bool consider_friendly) {
		std::vector<point> obstacles;
		const enemy_team &enemy(w->enemy);
		for (size_t i = 0; i < enemy.size(); ++i) {
			obstacles.push_back(enemy.get_robot(i)->position());
		}
		if (consider_friendly) {
			const friendly_team &friendly(w->friendly);
			for (size_t i = 0; i < friendly.size(); ++i) {
				const player::ptr fpl = friendly.get_player(i);
				if (fpl == pl) continue;
				obstacles.push_back(fpl->position());
			}
		}
		return calc_best_shot(w->field(), obstacles, pl->position());
	}

	double calc_goal_visibility_angle(const world::ptr w, const player::ptr pl, const bool consider_friendly) {
		return calc_best_shot(w, pl, consider_friendly).second;
	}

	std::vector<player::ptr> get_friends(const friendly_team& friendly, const std::vector<player::ptr>& exclude) {
		std::vector<player::ptr> friends;
		for (size_t i = 0; i < friendly.size(); ++i) {
			const player::ptr plr(friendly.get_player(i));
			if (exists(exclude.begin(), exclude.end(), plr)) continue;
			friends.push_back(plr);
		}
		return friends;
	}

	int choose_best_pass(const world::ptr w, const std::vector<player::ptr>& friends) {
		double bestangle = 0;
		double bestdist = 1e99;
		int bestidx = -1;
		for (size_t i = 0; i < friends.size(); ++i) {
			// see if this player is on line of sight
			if (!ai_util::can_receive(w, friends[i])) continue;
			// choose the most favourable distance
			const double dist = (friends[i]->position() - w->ball()->position()).len();
			const double angle = calc_goal_visibility_angle(w, friends[i]);
			if (bestidx == -1 || angle > bestangle || (angle == bestangle && dist < bestdist)) {
				bestangle = angle;
				bestdist = dist;
				bestidx = i;
			}
		}
		return bestidx;
	}

	point find_random_shoot_position(const world::ptr w) {
#warning TODO
		return w->field().enemy_goal();
	}

	bool has_ball(const world::ptr w, const player::ptr pl) {
		return pl->has_ball() || ball_front(w, pl);
	}

	bool posses_ball(const world::ptr w, const player::ptr pl) {
		//return pl->has_ball() || (pl->last_sense_ball_time() < HAS_BALL_ALLOWANCE && ball_close(w, pl));
		return pl->has_ball() || ball_close(w, pl);
	}

	// target is another player

}

