#include "ai/role/defensive2.h"
#include "ai/role/goalie.h"
#include "ai/tactic/pivot.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/receive.h"
#include "ai/util.h"
#include "util/algorithm.h"
#include "geom/util.h"

#include <iostream>

namespace {
	// minimum distance from the goal post
	const double MIN_GOALPOST_DIST = 0.05;
}

defensive2::defensive2(world::ptr world) : the_world(world) {
}

point defensive2::calc_goalie_pos(const bool top) const {
	const field& f = the_world->field();

	// maximum x-distance the goalie can go from own goal.
	const double maxdist = f.defense_area_radius() - robot::MAX_RADIUS;

	// there is ray ballpos to goalside
	const point& ballpos = the_world->ball()->position();
	const point goalside = top ? point(-f.length()/2, f.goal_width()/2) : point(-f.length(), -f.goal_width()/2);

	// NOTE! ensure that the ball is radius distance from the left end.

	// point L is the intersection of max goalpost distance and the ray
	const point L = line_intersect(goalside, ballpos, f.friendly_goal() + point(maxdist, -1), f.friendly_goal() + point(maxdist, 1));

	// block the cone from L
	if (top) {
		return L + calc_block_cone(goalside - ballpos, point(0, -1), robot::MAX_RADIUS);
	} else {
		return L + calc_block_cone(point(0, 1), goalside - ballpos, robot::MAX_RADIUS);
	}
}

// will always return at least 5 values
std::vector<point> defensive2::calc_block_positions(const bool top) const {
	const enemy_team& enemy(the_world->enemy);
	const field& f = the_world->field();

	// Sort enemies by distance from own goal.
	std::vector<robot::ptr> enemies = enemy.get_robots();
	std::sort(enemies.begin(), enemies.end(), ai_util::cmp_dist<robot::ptr>(the_world->ball()->position()));

	std::vector<point> waypoints;

	// there is cone ball to goal sides, bounded by 1 rays.
	const point& ballpos = the_world->ball()->position();
	const point goalside = top ? point(-f.length()/2, f.goal_width()/2) : point(-f.length(), -f.goal_width()/2);
	const point goalopp = top ? point(-f.length()/2, -f.goal_width()/2) : point(-f.length(), f.goal_width()/2);

	// goalie
	// maximum x-distance the goalie can go from own goal.
	const double maxdist = f.defense_area_radius() - robot::MAX_RADIUS;
	const point L = line_intersect(goalside, ballpos, f.friendly_goal() + point(maxdist, -1), f.friendly_goal() + point(maxdist, 1));
	const point G = (top) ?  L + calc_block_cone(goalside - ballpos, point(0, -1), robot::MAX_RADIUS)
		: L + calc_block_cone(point(0, 1), goalside - ballpos, robot::MAX_RADIUS);
	
	// first defender will block the remaining cone from the ball
	const point D1 = calc_block_cone_defender(goalside, goalopp, ballpos, G, robot::MAX_RADIUS);

	// 2nd defender block enemy sight to goal if needed

	// block another ray
	waypoints.push_back(G);
	waypoints.push_back(D1);

	return waypoints;
}

void defensive2::tick() {

	if (the_robots.size() == 0) return;

	const friendly_team& friendly(the_world->friendly);

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(the_robots.begin(), the_robots.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));
	std::vector<player::ptr> friends = ai_util::get_friends(friendly, the_robots);

	const int baller = ai_util::calc_baller(the_world, the_robots);
	const bool teamball = ai_util::friendly_posses_ball(the_world);

	// The robot that will do something to the ball (e.g. chase).
	// Other robots will just go defend or something.
	// TODO: maybe use refpointer instead of integer for safety reasons.
	int skipme = -1;

	if (teamball) {
		if (baller >= 0) {

			std::vector<player::ptr> nonballers;
			for (size_t i = 0; i < friendly.size(); ++i)
				if (friendly.get_player(i) != the_robots[baller])
					nonballers.push_back(friendly[i]);

			int passme = ai_util::choose_best_pass(the_world, nonballers);

			// TODO: do something
			if (passme == -1) {
				// ehh... nobody to pass to
				std::cout << "defensive2: " << the_robots[baller]->name << " has ball, shoot to the goal!" << std::endl;

				// try for the goal =D
				shoot::ptr shoot_tactic(new shoot(the_robots[baller], the_world));
				tactics[baller] = shoot_tactic;
			} else {
				std::cout << "defensive2: " << the_robots[baller]->name << " has ball, pass to " << nonballers[passme]->name << std::endl;

				// pass to this person
				pass::ptr pass_tactic(new pass(the_robots[baller], the_world, nonballers[passme]));
				tactics[baller] = pass_tactic;
			}

			skipme = baller;
		} else {
			// back to normal defensive position
		}
	} else {

		double frienddist = 1e99;
		for (size_t i = 0; i < friends.size(); ++i) {
			frienddist = std::min(frienddist, (friends[i]->position() - the_world->ball()->position()).len());
		}

		if ((the_robots[0]->position() - the_world->ball()->position()).len() < frienddist) {
			// std::cout << "defensive2: chase" << std::endl;
			// already sorted by distance to ball
			shoot::ptr shoot_tactic(new shoot(the_robots[0], the_world));
			tactics[0] = shoot_tactic;
			skipme = 0;
		} else {
			// std::cout << "defensive2: nothing special" << std::endl;
		}
	}

	std::vector<point> waypoints = calc_block_positions();

	std::vector<point> locations;
	for (size_t i = 0; i < the_robots.size(); ++i) {
		if (static_cast<int>(i) == skipme) continue;
		locations.push_back(the_robots[i]->position());
	}

	// ensure we are only blocking as we need
	while (waypoints.size() > locations.size()) waypoints.pop_back();

	std::vector<size_t> order = dist_matching(locations, waypoints);

	size_t w = 0;
	for (size_t i = 0; i < the_robots.size(); ++i) {
		if (static_cast<int>(i) == skipme) continue;
		if (w >= waypoints.size()) {
			std::cerr << "defender: " << the_robots[i]->name << " nothing to do!" << std::endl;
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

void defensive2::robots_changed() {
	tactics.clear();
	tactics.resize(the_robots.size());
}

