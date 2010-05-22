#include "ai/util.h"
#include "util/algorithm.h"

#include <iostream>

namespace {

#warning Magic constant
	const double SHOOT_ALLOWANCE = 1e-1;

}

namespace ai_util{

	const double ORI_CLOSE = 1e-2;

	const double POS_CLOSE = 1e-5;

	const unsigned int SHOOTING_SAMPLE_POINTS = 9;

	const std::vector<point> calc_candidates(const world::ptr world) {
		std::vector<point> candidates(SHOOTING_SAMPLE_POINTS);
		const field &the_field(world->field());

		const double goal_width = (the_field.goal_width() - robot::MAX_RADIUS) * 2;
		const double delta = goal_width / SHOOTING_SAMPLE_POINTS;

		for (size_t i = 0; i < SHOOTING_SAMPLE_POINTS; ++i) {
			point p(the_field.length(), -the_field.goal_width() + robot::MAX_RADIUS + i * delta);
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
		const double ballori = (ball->position() - passee->position()).orientation();
		if (std::fmod(std::abs(ballori - passee->orientation()), M_PI) > ORI_CLOSE) return false;
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

	std::vector<robot::ptr> get_robots(const team& theteam) {
		std::vector<robot::ptr> robots(theteam.size());
		for (size_t i = 0; i < theteam.size(); ++i) {
			robots[i] = theteam.get_robot(i);
		}
		return robots;
	}

	std::vector<player::ptr> get_players(const friendly_team& friendly) {
		std::vector<player::ptr> players(friendly.size());
		for (size_t i = 0; i < friendly.size(); ++i) {
			players[i] = friendly.get_player(i);
		}
		return players;
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

}

