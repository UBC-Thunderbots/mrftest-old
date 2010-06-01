#include "ai/role/offensive.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/receive.h"
#include "ai/util.h"

namespace {

	const double SHOOT_ALLOWANCE = ball::RADIUS;

	const double ENEMY_FACTOR = 1.0;
	const double CAN_SEE_BALL = 1.0;

	// chop up half the field into 100x100 grid
	// evaluate some functions

	const int EV_Y = 100;
	const int EV_X = 100;

};

offensive::offensive(world::ptr world) : the_world(world) {
}

/**
 * The scoring function for having the robot in the particular position.
 */
double offensive::calc_position_score(const std::vector<robot::ptr>& enemies, const point& pos) const {
	// Hmm.. not sure if having negative number is a good idea.
	double score = ai_util::calc_goal_visibility_angle(the_world->field(), enemies, pos);
	// distance to enemies
	for (size_t i = 0; i < enemies.size(); ++i) {
		double dist = (pos - enemies[i]->position()).len();
		// too close!
		if(dist < robot::MAX_RADIUS) return -1e99;
		score += -1.0 / (dist + 1.0);
	}
	// TODO: magic constants
	// whether this point can see the ball
	if (ai_util::path_check(the_world->ball()->position(), pos, enemies, robot::MAX_RADIUS + ball::RADIUS + SHOOT_ALLOWANCE)) {
		score += 1.0;
	}
	return score;
}

/**
 * Assume that role has the ball.
 * Given:
 * - enemy positions
 * - ball
 * Find where to position the robot so that it has the greatest chance of shooting.
 * The enemy position is provided as vector so we can add imaginary enemies.
 * If no position is valid, will simply choose the middle of the field.
 */
point offensive::calc_position_best(const std::vector<robot::ptr>& enemies) const {
	const double x1 = 0;
	const double x2 = the_world->field().length();
	const double y1 = -the_world->field().width() / 2;
	const double y2 = the_world->field().width() / 2;

	const double dx = (x2 - x1) / (EV_X+1);
	const double dy = (y2 - y1) / (EV_Y+1);
	double bestscore = 0;
	point bestpos(0, 0);
	for(int j = 0; j < EV_Y; ++j) {
		for(int i = 0; i < EV_X; ++i) {
			const double x = x1 + dx * (i + 1);
			const double y = y1 + dy * (j + 1);
			const point pos = point(x, y);
			const double score = calc_score(enemies, pos);
			if(score > bestscore) {
				bestscore = score;
				bestpos = pos;
			}
		}
	}
	return bestpos;
}

void offensive::move_towards_goal(int index) {
	move::ptr move_tactic(new move(the_robots[index], the_world));
	move_tactic->set_position(the_world->field().enemy_goal());
	the_tactics[index] = move_tactic;
}

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

	int baller = -1;
	for(size_t i = 0; i < the_robots.size(); i++) {
		if(the_robots[i]->has_ball()) {
			baller = i;
			break;
		}
	}

	bool haveball = false;
	for(size_t i = 0; i < friendly.size(); i++) {
		if(friendly.get_player(i)->has_ball()) {
			haveball = true;
			break;
		}
	}

	// std::vector<player::ptr> friends = ai_util::get_friends(friendly, the_robots);

	if (haveball) {
		// someone has the ball
		if (baller != -1) {
			// someone in this role has the ball
			if(get_distance_from_goal(baller) < the_world->field().length() / 3) {
				if (ai_util::calc_best_shot(the_robots[baller],the_world) < ai_util::SHOOTING_SAMPLE_POINTS) {
					the_tactics[baller] = shoot::ptr(new shoot(the_robots[baller], the_world));
				} else {
					int best_passee = -1;
					double best_dist = 1e99;
					// We will try passing to another offensive robot,
					// if there is a clear path to the passee and the passee has a clear path to the goal
					for (size_t j = 0; j < the_robots.size(); j++) {
						if (static_cast<int>(j) == baller) continue;  
						if (ai_util::can_pass(the_world,the_robots[j]) && ai_util::calc_best_shot(the_robots[j],the_world) < ai_util::SHOOTING_SAMPLE_POINTS) {
							double new_dist = (the_robots[j]->position()-the_robots[baller]->position()).len() + get_distance_from_goal(j);
							if (best_dist > new_dist) {
								best_dist = new_dist;
								best_passee = j;
							}
						}
					}
					if (best_passee != -1) {
						// found suitable passee, make a pass
						the_tactics[baller] = pass::ptr(new pass(the_robots[baller], the_world, the_robots[best_passee]));
					} else if (get_distance_from_goal(baller) < the_world->field().length()/6) {
						// very close to goal, so try making a shot anyways
						the_tactics[baller] = shoot::ptr(new shoot(the_robots[baller], the_world));
					} else {
						// dribble towards the goal
						// TODO: can't dribble for too long
						move_towards_goal(baller);
						// the_tactics[baller] = move::ptr(new move(the_robots[baller], the_world, the_world->field().enemy_goal()));
					}
				}
			} else {
				// dribble the ball slowly to goal
				// TODO: can't dribble for too long
				move_towards_goal(baller);
			}
			for (size_t i = 0; i < the_robots.size(); i++) {
				if (i == baller) continue;
				move_towards_goal(i);
			}
		} else {
			// no one in this role has the ball
			// prepare to receive some ball
			for (size_t i = 0; i < the_robots.size(); i++) {
				the_tactics[i] = receive::ptr(new receive(the_robots[i], the_world));
			}
		}
	} else {
		// no one has the ball
		for(size_t i = 0; i < the_robots.size(); i++) {
			the_tactics[i] = chase::ptr(new chase(the_robots[i], the_world));
		}
	}

    unsigned int flags = ai_flags::calc_flags(the_world->playtype());
    if (haveball) flags |= ai_flags::clip_play_area;
    
	for(size_t i = 0; i < the_tactics.size(); i++) {
		the_tactics[i]->tick();
		the_tactics[i]->set_flags(flags);
    }
}

void offensive::robots_changed() {
	the_tactics.clear();
	the_tactics.resize(the_robots.size());
	tick();
}

