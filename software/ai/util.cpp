#include "ai/util.h"
#include "geom/angle.h"

#include <iostream>

namespace {

#warning Magic constant
	const double SHOOT_ALLOWANCE = 1e-1;

}

namespace ai_util{

	const double ORI_CLOSE = 1e-2;

	const double POS_CLOSE = 1e-5;

	const unsigned int SHOOTING_SAMPLE_POINTS = 9;

	double orientation(const point& p) {
		return atan2(p.x, p.y);
	}

	double angle_diff(const double& a, const double& b) {
		return fmod(abs(a - b), PI);
	}

	const std::vector<point> calc_candidates(const world::ptr world){
		std::vector<point> candidates(SHOOTING_SAMPLE_POINTS);
		const field &the_field(world->field());

		// allow some space for the ball to go in from the post
		const double EDGE_SPACE = 0.1;

		double goal_width = (the_field.goal_width() - EDGE_SPACE) * 2;
		double delta = goal_width / SHOOTING_SAMPLE_POINTS;

		for (unsigned int i = 0; i < SHOOTING_SAMPLE_POINTS; ++i) {
			point p(the_field.length(), -the_field.goal_width() + EDGE_SPACE + i * delta);
			candidates[i] = p;
		}
		return candidates;
	}

	bool path_check(const point& begin, const point& end, const team& theteam, const double& thresh, const robot::ptr skip) {
		const point direction = (end - begin).norm();
		const double dist = (end - begin).len();
		for (size_t i = 0; i < theteam.size(); ++i) {
			const robot::ptr rob = theteam.get_robot(i);
			if (rob == skip) continue;
			const point rp = rob->position() - end;
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);
			if (proj <= 0) continue;
			if (proj < dist && perp < thresh) {
				return false;
			}
		}
		return true;
	}

	bool path_check(const point& begin, const point& end, const team& theteam, const double& thresh) {
		const point direction = (end - begin).norm();
		const double dist = (end - begin).len();
		for (size_t i = 0; i < theteam.size(); ++i) {
			const robot::ptr rob = theteam.get_robot(i);
			const point rp = rob->position() - end;
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);
			if (proj <= 0) continue;
			if (proj < dist && perp < thresh) {
				return false;
			}
		}
		return true;
	}

	bool can_pass(const world::ptr w, const player::ptr passee) {
		const ball::ptr ball = w->ball();
		if ((ball->position() - passee->position()).lensq() < POS_CLOSE) {
			std::cerr << "can_pass: passe too close to ball" << std::endl;
			return true;
		}
		// if the passee is not facing the ball, forget it
		const double ballori = orientation(ball->position() - passee->position());
		if (std::fmod(std::abs(ballori - passee->orientation()), PI) > ORI_CLOSE) return false;
		// check if there is some enemy blocking
		// if(!path_check(ball->position(), passee->position(), w->enemy, SHOOT_ALLOWANCE + robot::MAX_RADIUS + ball::RADIUS)) return false;
		const point direction = (ball->position() - passee->position()).norm();
		const double dist = (ball->position() - passee->position()).len();
		for (size_t i = 0; i < w->enemy.size(); ++i) {
			const robot::ptr rob = w->enemy.get_robot(i);
			const point rp = rob->position() - passee->position();
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);
			if (proj <= 0) continue;
			if (proj < dist && perp < SHOOT_ALLOWANCE + robot::MAX_RADIUS + ball::RADIUS) {
				return false;
			}
		}
		for (size_t i = 0; i < w->friendly.size(); ++i) {
			const player::ptr plr = w->friendly.get_player(i);
			if (plr->has_ball() || plr == passee) continue;
			const point rp = plr->position() - passee->position();
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);
			if (proj <= 0) continue;
			if (proj < dist && perp < SHOOT_ALLOWANCE + robot::MAX_RADIUS + ball::RADIUS) {
				return false;
			}
		}
		return true;
	}

	size_t calc_best_shot(const player::ptr player, const world::ptr world) {
		std::vector<point> candidates = calc_candidates(world);
		size_t best_point = candidates.size();
		double best_score = -1;

		const team &opponent_team(world->enemy);
		double proximity, score, dist;

		for (unsigned int i = 0; i < SHOOTING_SAMPLE_POINTS; ++i) {		
			point projection = candidates[i] - player->position();
			score = 0;
			for (unsigned int i = 0; i < opponent_team.size(); ++i) {
#warning TODO: take into account of velocity?
				point other = opponent_team.get_robot(i)->position() - player->position();
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

					if (best_point == candidates.size() || score < best_score) {
						best_point = i;
						best_score = score;
					}
				}
			}
		}
		return best_point;
	}

	point clip_point(const point& p, const point& bound1, const point& bound2) {

		double minx = std::min(bound1.x, bound2.x);
		double miny = std::min(bound1.y, bound2.y);
		double maxx = std::max(bound1.x, bound2.x);
		double maxy = std::max(bound1.y, bound2.y);

		point ret = p;

		if (p.x < minx) ret.x = minx;
		else if (p.x > maxx) ret.x = maxx;      

		if (p.y < miny) ret.y = miny;
		else if (p.y > maxy) ret.y = maxy;

		return ret;
	}

}

