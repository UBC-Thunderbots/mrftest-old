#include "ai/util.h"
#include "util/algorithm.h"
#include "geom/angle.h"
#include <cmath>

#include <iostream>

namespace {

#warning Magic constant
	const double SHOOT_ALLOWANCE = 1e-1;

	const double PI = M_PI;
}

namespace ai_util{

	const double HAS_BALL_ALLOWANCE = 3.0;

	const unsigned int SHOOTING_SAMPLE_POINTS = 9;

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

	bool ball_close(const world::ptr w, const player::ptr p) {
		const point dist = w->ball()->position() - p->position();
		if (dist.len() > p->MAX_RADIUS + w->ball()->RADIUS*2) return false;
		return angle_diff(dist.orientation(), p->orientation()) < PI / 2;
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

	bool can_pass(const world::ptr w, const player::ptr passee) {
		const ball::ptr ball = w->ball();
		if ((ball->position() - passee->position()).lensq() < POS_CLOSE) {
			std::cerr << "can_pass: passe too close to ball" << std::endl;
			return true;
		}
		// if the passee is not facing the ball, forget it
		const double ballori = (ball->position() - passee->position()).orientation();
		if (std::fmod(std::abs(ballori - passee->orientation()), M_PI) > ORI_CLOSE) return false;
		// check if there is some enemy blocking
		// if(!path_check(ball->position(), passee->position(), w->enemy.get_robots(), SHOOT_ALLOWANCE + robot::MAX_RADIUS + ball::RADIUS)) return false;
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

	int calc_best_shot(const player::ptr player, const world::ptr world) {
		std::vector<point> candidates = calc_candidates(world);
		int best_point = -1;
		double best_score = -1;

		const team &opponent_team(world->enemy);
		double proximity, score, dist;

		for (unsigned int i = 0; i < SHOOTING_SAMPLE_POINTS; ++i) {		
			point projection = candidates[i] - player->position();
			score = 0;
			for (unsigned int j = 0; j < opponent_team.size(); ++j) {
#warning TODO: take into account of velocity?
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

	std::vector<player::ptr> get_friends(const friendly_team& friendly, const std::vector<player::ptr>& exclude) {
		std::vector<player::ptr> friends;
		for (size_t i = 0; i < friendly.size(); ++i) {
			const player::ptr plr(friendly.get_player(i));
			if (exists(exclude.begin(), exclude.end(), plr)) continue;
			friends.push_back(plr);
		}
		return friends;
	}

	std::vector<size_t> dist_matching(const std::vector<point>& v1, const std::vector<point>& v2) {
		std::vector<size_t> order(v2.size());
		for (size_t i = 0; i < v2.size(); ++i) order[i] = i;
		std::vector<size_t> best = order;
		double bestscore = 1e99;
		const size_t N = std::min(v1.size(), v2.size());
		do {
			double score = 0;
			for (size_t i = 0; i < N; ++i) {
				score += (v1[i] - v2[order[i]]).len();
			}
			if (score < bestscore) {
				bestscore = score;
				best = order;
			}
		} while(std::next_permutation(order.begin(), order.end()));
		return best;
	}

	int choose_best_pass(const world::ptr w, const std::vector<player::ptr>& friends) {
		double neardist = 1e99;
		int nearidx = -1;
		for (size_t i = 0; i < friends.size(); ++i) {
			// see if this player is on line of sight
			if (!ai_util::can_pass(w, friends[i])) continue;
			// choose the most favourable distance
			const double dist = (friends[i]->position() - w->ball()->position()).len();
			if (nearidx == -1 || dist < neardist) {
				neardist = dist;
				nearidx = i;
			}
		}
		return nearidx;
	}

	// this function may be replaced in the future
	double calc_goal_visibility_angle(const world::ptr w, const point& p, bool consider_friendly){
		std::vector<std::pair<double, int> > events;
		double l_range = (point(w->field().length()/2.0,-w->field().goal_width()/2.0) - p).orientation();
		double h_range = (point(w->field().length()/2.0,w->field().goal_width()/2.0) - p).orientation();
		events.push_back(std::make_pair(l_range,1));
		events.push_back(std::make_pair(h_range,-1));

		size_t lim = w->enemy.size();
		if (consider_friendly) lim += w->friendly.size();
		for (size_t i = 0; i < lim; ++i){
			point diff;
			if (i < w->enemy.size()) diff = w->enemy.get_robot(i)->position() - p;
			else diff = w->friendly.get_robot(i-w->enemy.size())->position() - p;
			if (diff.len() < robot::MAX_RADIUS + ORI_CLOSE)
				return -2*acos(-1);
			double cent = diff.orientation();
			double span = asin(robot::MAX_RADIUS / diff.len());
			events.push_back(std::make_pair(cent-span,-1));
			events.push_back(std::make_pair(cent+span,1));
		}
		sort(events.begin(),events.end());
		double best = 0;
		double sum = 0;
		double cnt = 0;
		for (size_t i = 0; i < events.size() - 1; i++){
			cnt += events[i].second;
			if (cnt > 0){
				sum += events[i+1].first - events[i].first;
				if (best < sum) best = sum;
			} else
				sum = 0;
		}
		return sum;
	}

	double calc_goal_visibility_angle(const field& f, const std::vector<point>& obstacles, const point& p) {
		std::vector<std::pair<double, int> > events;
		double l_range = (point(f.length()/2.0,-f.goal_width()/2.0) - p).orientation();
		double h_range = (point(f.length()/2.0,f.goal_width()/2.0) - p).orientation();
		events.push_back(std::make_pair(l_range,1));
		events.push_back(std::make_pair(h_range,-1));
		for (size_t i = 0; i < obstacles.size(); ++i) {
			point diff = obstacles[i] - p;
			if (diff.len() < robot::MAX_RADIUS + ORI_CLOSE) {
				//return -2*acos(-1);
				return 0;
			}
			double cent = diff.orientation();
			double span = asin(robot::MAX_RADIUS / diff.len());
			events.push_back(std::make_pair(cent-span,-1));
			events.push_back(std::make_pair(cent+span,1));
		}
		sort(events.begin(),events.end());
		double best = 0;
		double sum = 0;
		double cnt = 0;
		for (size_t i = 0; i + 1 < events.size(); i++) {
			cnt += events[i].second;
			if (cnt > 0) {
				sum += events[i+1].first - events[i].first;
				if (best < sum) best = sum;
			} else
				sum = 0;
		}
		return sum;
	}

	bool posses_ball(const world::ptr w, const player::ptr pl) {
		return pl->has_ball() || pl->has_ball_time() < HAS_BALL_ALLOWANCE || ball_close(w, pl);
	}

}

