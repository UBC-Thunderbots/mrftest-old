#include "ai/role/offensive.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/dribble.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/receive.h"
#include "ai/util.h"

#include <iostream>

namespace {

	const double SHOOT_ALLOWANCE = ball::RADIUS;

	const double ENEMY_FACTOR = 1.0;
	const double CAN_SEE_BALL = 1.0;

	// chop up half the field into 100x100 grid
	// evaluate some functions

	const int GRIDY = 100;
	const int GRIDX = 100;

};

offensive::offensive(world::ptr world) : the_world(world) {
}

double offensive::scoring_function(const std::vector<point>& enemypos, const point& pos) const {
	// Hmm.. not sure if having negative number is a good idea.
	double score = ai_util::calc_goal_visibility_angle(the_world->field(), enemypos, pos);
	// distance to enemies
	for (size_t i = 0; i < enemypos.size(); ++i) {
		double dist = (pos - enemypos[i]).len();
		// too close!
		if(dist < robot::MAX_RADIUS) return -1e99;
		score += -1.0 / (dist + 1.0);
	}
	// TODO: magic constants
	// whether this point can see the ball
	if (ai_util::path_check(the_world->ball()->position(), pos, enemypos, robot::MAX_RADIUS + ball::RADIUS + SHOOT_ALLOWANCE)) {
		score += 1.0;
	}
	return score;
}

point offensive::calc_position_best(const std::vector<point>& enemypos) const {
	const double x1 = 0;
	const double x2 = the_world->field().length();
	const double y1 = -the_world->field().width() / 2;
	const double y2 = the_world->field().width() / 2;

	const double dx = (x2 - x1) / (GRIDX+1);
	const double dy = (y2 - y1) / (GRIDY+1);
	double bestscore = 0;
	point bestpos(0, 0);
	for (int j = 0; j < GRIDY; ++j) {
		for (int i = 0; i < GRIDX; ++i) {
			const double x = x1 + dx * (i + 1);
			const double y = y1 + dy * (j + 1);
			const point pos = point(x, y);
			const double score = scoring_function(enemypos, pos);
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
	std::vector<point> ret;
	for (size_t i = 0; i < n; ++i) {
		const point best = calc_position_best(enemypos);
		ret.push_back(best);
		enemypos.push_back(best);
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
	if (the_robots.size() == 0) return;

	// Sort by distance to ball. DO NOT SORT AGAIN!!
	std::sort(the_robots.begin(), the_robots.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));

	const friendly_team& friendly(the_world->friendly);

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

			std::vector<size_t> order = ai_util::dist_matching(locations, waypoints);

			size_t w = 0;
			for (size_t i = 0; i < the_robots.size(); ++i) {
				if (static_cast<int>(i) == baller) continue;
				if (w >= waypoints.size()) {
					std::cerr << "Offender has nothing to do!" << std::endl;
					move::ptr move_tactic(new move(the_robots[i], the_world));
					move_tactic->set_position(the_robots[i]->position());
					tactics[i] = move_tactic;
				} else {
					move::ptr move_tactic(new move(the_robots[i], the_world));
					move_tactic->set_position(waypoints[order[w]]);
					tactics[i] = move_tactic;
				}
			}

			// as for the robot with the ball, make the robot try to shoot the goal, or pass to someone
			if (get_distance_from_goal(baller) < the_world->field().length() / 3) {
				if (ai_util::calc_best_shot(the_robots[baller],the_world) != -1) {
					tactics[baller] = shoot::ptr(new shoot(the_robots[baller], the_world));
				} else {
					int passme = -1;
					double passbest = 1e99;
					// We will try passing to another offensive robot,
					// if there is a clear path to the passee and the passee has a clear path to the goal
					for (size_t j = 0; j < the_robots.size(); ++j) {
						if (static_cast<int>(j) == baller) continue;  
						if (ai_util::can_pass(the_world, the_robots[j]) && ai_util::calc_best_shot(the_robots[j], the_world) != -1) {
							double dist = (the_robots[j]->position() - the_robots[baller]->position()).len() + get_distance_from_goal(j);
							if (passbest > dist) {
								passbest = dist;
								passme = j;
							}
						}
					}
					if (passme != -1) {
						// found suitable passee, make a pass
						tactics[baller] = pass::ptr(new pass(the_robots[baller], the_world, the_robots[passme]));
					} else if (get_distance_from_goal(baller) < the_world->field().length()/6) {
						// very close to goal, so try making a shot anyways
						tactics[baller] = shoot::ptr(new shoot(the_robots[baller], the_world));
					} else {
						// dribble towards the goal
						// TODO: can't dribble for too long
#warning do something more intelligent than just moving towards goal
						dribble::ptr dribble_tactic(new dribble(the_robots[baller], the_world));
						dribble_tactic->set_position(the_world->field().enemy_goal());
						tactics[baller] = dribble_tactic;
					}
				}
			} else {
				// dribble the ball slowly to goal
				// TODO: can't dribble for too long
#warning do something more intelligent than just moving towards goal
				dribble::ptr dribble_tactic(new dribble(the_robots[baller], the_world));
				dribble_tactic->set_position(the_world->field().enemy_goal());
				tactics[baller] = dribble_tactic;
			}
		} else {
			// no one in this role has the ball
			// prepare to receive some ball
			for (size_t i = 0; i < the_robots.size(); ++i) {
				tactics[i] = receive::ptr(new receive(the_robots[i], the_world));
			}
		}
	} else {
		// no one has the ball
		// just do chase for now
		for (size_t i = 0; i < the_robots.size(); ++i) {
			tactics[i] = chase::ptr(new chase(the_robots[i], the_world));
		}
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

