#include "ai/role/offensive.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/pivot.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/dribble.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/receive.h"
#include "ai/util.h"
#include "geom/angle.h"
#include "geom/util.h"

#include <iostream>

namespace {

	const double SHOOT_ALLOWANCE = ball::RADIUS;

	const double ENEMY_FACTOR = 1.0;
	const double CAN_SEE_BALL = 1.0;

	// chop up half the field into 100x100 grid
	// evaluate some functions

	const int GRIDY = 100;
	const int GRIDX = 100;

	const double ONE = 1.0 / 180.0 * M_PI;

	const double NEAR = robot::MAX_RADIUS * 3;

};

offensive::offensive(world::ptr world) : the_world(world) {
}

double offensive::scoring_function(const std::vector<point>& enemypos, const point& pos, const std::vector<point>& dontblock) const {
	// Hmm.. not sure if having negative number is a good idea.
	std::pair<point, double> bestshot = ai_util::calc_best_shot(the_world->field(), enemypos, pos);
	double score = bestshot.second;

	for (size_t i = 0; i < dontblock.size(); ++i) {
		std::pair<point, double> shootershot = ai_util::calc_best_shot(the_world->field(), enemypos, dontblock[i]);
		const point diff1 = (shootershot.first - dontblock[i]);
		const point diff2 = (pos - dontblock[i]);
		if (diff2.len() < NEAR) {
			return -1e99;
		}
		const double anglediff = angle_diff(diff1.orientation(), diff2.orientation());
		if (anglediff * 2 < shootershot.second) {
			return -1e99;
		}
	}

	// TODO: check the line below here
	// scoring factors:
	// density of enemy, passing distance, distance to the goal, angle of shooting, angle of receiving
	// distance toward the closest enemy, travel distance, behind of in front of the enemy
	// UI for viewing
	if (!ai_util::path_check(the_world->ball()->position(), pos, enemypos, robot::MAX_RADIUS + ball::RADIUS + SHOOT_ALLOWANCE)) {
		return -1e99;
	}

	// 10 degrees of shooting is 10 points
	score *= 10.0 / (10.0 * ONE);
	// want to be as near to our own goal as possible
	// score -= 1.0 * pos.x;
	const double balldist = (pos - the_world->ball()->position()).len();
	const double goaldist = (pos - bestshot.first).len();

	// divide by largest distance?
	const double bigdist = std::max(balldist, goaldist);
	score /= bigdist;

	return score;
}

point offensive::calc_position_best(const std::vector<point>& enemypos, const std::vector<point>& dontblock) const {
	const double x1 = 0;
	const double x2 = the_world->field().length() / 2;
	const double y1 = -the_world->field().width() / 2;
	const double y2 = the_world->field().width() / 2;

	const double dx = (x2 - x1) / (GRIDX+1);
	const double dy = (y2 - y1) / (GRIDY+1);
	double bestscore = -1e50;
	point bestpos(0, 0);
	for (int j = 0; j < GRIDY; ++j) {
		for (int i = 0; i < GRIDX; ++i) {
			const double x = x1 + dx * (i + 1);
			const double y = y1 + dy * (j + 1);
			const point pos = point(x, y);
			// TEMPORARY HACK!!
			const double goaldist = (pos - the_world->field().enemy_goal()).len();
			if (goaldist < the_world->field().goal_width()) continue;
			const double score = scoring_function(enemypos, pos, dontblock);
			if (score > bestscore) {
				bestscore = score;
				bestpos = pos;
			}
		}
	}
	return bestpos;
}

std::vector<point> offensive::calc_position_best(const unsigned int n) const {
	const enemy_team& enemy = the_world->enemy;
	std::vector<point> enemypos;
	for (size_t i = 0; i < enemy.size(); ++i) {
		enemypos.push_back(enemy.get_robot(i)->position());
	}
	std::vector<point> dontblock;
	dontblock.push_back(the_world->ball()->position());
	std::vector<point> ret;
	for (size_t i = 0; i < n; ++i) {
		const point best = calc_position_best(enemypos, dontblock);
		ret.push_back(best);
		dontblock.push_back(best);
	}
	return ret;
}

// TODO: refactor
void offensive::move_towards_goal(int index) {
	move::ptr move_tactic(new move(the_robots[index], the_world));
	move_tactic->set_position(the_world->field().enemy_goal());
	tactics[index] = move_tactic;
}

// TODO: refactor
double offensive::get_distance_from_goal(int index) const {
	point pos = the_robots[index]->position();
	point goal = point(the_world->field().length()/2,0);
	point dist = goal-pos;
	double distance = dist.len();
	return distance;
}

void offensive::tick() {
	tick(Cairo::RefPtr<Cairo::Context> ());
}

void offensive::tick(Cairo::RefPtr<Cairo::Context> overlay) {
	if (the_robots.size() == 0) return;

	// Sort by distance to ball. DO NOT SORT AGAIN!!
	std::sort(the_robots.begin(), the_robots.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));

	const friendly_team& friendly(the_world->friendly);
	const field& the_field = the_world->field();

	bool teampossesball = false;
	int baller = -1;
	for (size_t i = 0; i < the_robots.size(); ++i) {
		if (ai_util::posses_ball(the_world, the_robots[i])) {
			baller = i;
			teampossesball = true;
			break;
		}
	}

	std::vector<player::ptr> friends = ai_util::get_friends(friendly, the_robots);

	if (!teampossesball) {
		for (size_t i = 0; i < friends.size(); ++i) {
			if (ai_util::posses_ball(the_world, friends[i])) {
				teampossesball = true;
				break;
			}
		}
	}

	if (baller != -1 && baller != 0) {
		std::cerr << "offensive: nearest robot don't have the ball??" << std::endl;
	}

	if (teampossesball) {
		// someone has the ball
		if (baller != -1) {
			// calculate some good positions for robots not holding the ball
			std::vector<point> waypoints = calc_position_best(static_cast<int>(the_robots.size()) - 1);

			// other robots not having the ball
			std::vector<player::ptr> available;
			std::vector<point> locations;
			for (size_t i = 0; i < the_robots.size(); ++i) {
				if (static_cast<int>(i) == baller) continue;
				available.push_back(the_robots[i]);
				locations.push_back(the_robots[i]->position());
			}

			std::vector<size_t> order = dist_matching(locations, waypoints);

			size_t w = 0;
			for (size_t i = 0; i < the_robots.size(); ++i) {
				if (static_cast<int>(i) == baller) continue;
				if (w >= waypoints.size()) {
					std::cout << "offender: nothing to do!" << std::endl;
					move::ptr move_tactic(new move(the_robots[i], the_world));
					move_tactic->set_position(the_robots[i]->position());
					tactics[i] = move_tactic;
				} else {
					move::ptr move_tactic(new move(the_robots[i], the_world));
					move_tactic->set_position(waypoints[order[w]]);
					tactics[i] = move_tactic;
				}
				++w;
			}

			// as for the robot with the ball, make the robot try to shoot the goal, or pass to someone
			if (!the_robots[baller]->has_ball()) {
				// std::cout << "offensive: has no ball but posses, chase" << std::endl;
				tactics[baller] = pivot::ptr(new pivot(the_robots[baller], the_world));
			} else {
				int shooter = -1;
				double shooterangle = 0;

				// We will try passing to another offensive robot,
				// if there is a clear path to the passee and the passee has a clear path to the goal
				for (size_t j = 0; j < the_robots.size(); ++j) {
					if (static_cast<int>(j) != baller && !ai_util::can_receive(the_world, the_robots[j])) continue;
					// if (ai_util::calc_best_shot(the_robots[j], the_world) == -1) continue;
					// if (get_distance_from_goal(j) > the_world->field().length() / 2) continue;

					// TODO: create another weighting function
					double angle = ai_util::calc_goal_visibility_angle(the_world, the_robots[j], false);
					std::cout << " j=" << j << " angle=" << (angle * 180.0 / M_PI) << std::endl;
					// the baller has more importance
					if (j == baller) angle *= 10.0;
					if (shooter == -1 || angle > shooterangle) {
						shooter = j;
						shooterangle = angle;
					}
				}

				//if (overlay) overlay->move_to(the_robots[baller]->position().x, the_robots[baller]->position().y);

				std::cout << "offensive: who to shoot " << shooter << std::endl;
				if (shooter == baller) {
					// i shall shoot
					tactics[baller] = shoot::ptr(new shoot(the_robots[baller], the_world));
					//if (overlay) overlay->line_to(the_field.enemy_goal().x, the_field.enemy_goal().y);
				} else if (shooter != -1) {
					// found suitable passee, make a pass
					tactics[baller] = pass::ptr(new pass(the_robots[baller], the_world, the_robots[shooter]));
				} else if (get_distance_from_goal(baller) < the_world->field().length() / 6) {
					// very close to goal, so try making a shot anyways
					tactics[baller] = shoot::ptr(new shoot(the_robots[baller], the_world));
				} else {
					// dribble towards the goal
					//dribble::ptr dribble_tactic(new dribble(the_robots[baller], the_world));
					//dribble_tactic->set_position(the_world->field().enemy_goal());
					//tactics[baller] = dribble_tactic;
					// i shall shoot
					tactics[baller] = shoot::ptr(new shoot(the_robots[baller], the_world));
				}
			}
		} else {
			// no one in this role has the ball
			// prepare to receive some ball
			for (size_t i = 0; i < the_robots.size(); ++i) {
				tactics[i] = receive::ptr(new receive(the_robots[i], the_world));
			}
		}
	} else {
		// calculate some good positions for robots not holding the ball
		std::vector<point> waypoints = calc_position_best(static_cast<int>(the_robots.size()) - 1);

		// other robots not having the ball
		std::vector<player::ptr> available;
		std::vector<point> locations;
		for (size_t i = 1; i < the_robots.size(); ++i) {
			available.push_back(the_robots[i]);
			locations.push_back(the_robots[i]->position());
		}

		std::vector<size_t> order = dist_matching(locations, waypoints);

		size_t w = 0;
		for (size_t i = 1; i < the_robots.size(); ++i) {
			if (w >= waypoints.size()) {
				std::cout << "offender: nothing to do!" << std::endl;
				move::ptr move_tactic(new move(the_robots[i], the_world));
				move_tactic->set_position(the_robots[i]->position());
				tactics[i] = move_tactic;
			} else {
				move::ptr move_tactic(new move(the_robots[i], the_world));
				move_tactic->set_position(waypoints[order[w]]);
				tactics[i] = move_tactic;
			}
			++w;
		}

		// std::cout << "offensive: chase " << std::endl;
		tactics[0] = pivot::ptr(new pivot(the_robots[0], the_world));
		// no one has the ball
		// just do chase for now
		// for (size_t i = 0; i < the_robots.size(); ++i) {
		//tactics[i] = pivot::ptr(new pivot(the_robots[i], the_world));
		//tactics[i] = chase::ptr(new chase(the_robots[i], the_world));
		//}
	}

	unsigned int flags = ai_flags::calc_flags(the_world->playtype());

	for (size_t i = 0; i < tactics.size(); ++i) {
		if (static_cast<int>(i) == baller) {
			tactics[i]->set_flags(flags | ai_flags::clip_play_area);
		} else {
			tactics[i]->set_flags(flags);
		}
		tactics[i]->tick();
	}
}

void offensive::robots_changed() {
	tactics.clear();
	tactics.resize(the_robots.size());
}

