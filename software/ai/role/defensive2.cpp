#include "ai/role/defensive2.h"
#include "ai/role/offensive.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/pass.h"
#include "ai/util.h"
#include "util/algorithm.h"
#include "geom/util.h"
#include "util/dprint.h"

#include "uicomponents/param.h"

#include <iostream>

namespace {
	// minimum distance from the goal post
	double_param MIN_GOALPOST_DIST("Defensive2 distance to goal post", 0.03, 0.03, 1.0);
	const double MAX_GOALIE_DIST = robot::MAX_RADIUS * 2;

	bool_param USE_GOALIE_RUSH("defensive2: use goalie rush when ball threatening", FALSE);
	double_param BALL_DANGEROUS_SPEED("defensive2: threatening ball horizontal speed", 3.0, 1.0, 10.0); 

	// used to save if the goalie should be on the top or bottom side
	class defensive2_state : public player::state {
		public:
			typedef Glib::RefPtr<defensive2_state> ptr;
			defensive2_state() : top(false) { }
			bool top;
	};

}

defensive2::defensive2(world::ptr world) : the_world(world) {
}

void defensive2::assign(const player::ptr& p, tactic::ptr t) {
	for (size_t i = 0; i < the_robots.size(); ++i) {
		if (the_robots[i] == p) {
			tactics[i] = t;
			return;
		}
	}
	LOG_ERROR("assign unknown robot");
}

std::pair<point, std::vector<point> > defensive2::calc_block_positions(const bool top) const {
	const enemy_team& enemy(the_world->enemy);

	const field& f = the_world->field();

	// Sort enemies by distance from own goal.
	std::vector<robot::ptr> enemies = enemy.get_robots();
	//std::sort(enemies.begin(), enemies.end(), ai_util::cmp_dist<robot::ptr>(the_world->ball()->position()));
	std::sort(enemies.begin(), enemies.end(), ai_util::cmp_dist<robot::ptr>(f.friendly_goal()));

	std::vector<point> waypoints;

	// there is cone ball to goal sides, bounded by 1 rays.
	const point& ballpos = the_world->ball()->position();
	const point goalside = top ? point(-f.length()/2, f.goal_width()/2) : point(-f.length()/2, -f.goal_width()/2);
	const point goalopp = top ? point(-f.length()/2, -f.goal_width()/2) : point(-f.length()/2, f.goal_width()/2);

	point G;
	/*
	   {
	// goalie and first defender integrated defence
	// maximum x-distance the goalie can go from own goal.
	// normally
	const double maxdist = f.defense_area_radius();
	const point L = line_intersect(goalside, ballpos, f.friendly_goal() + point(maxdist, -1), f.friendly_goal() + point(maxdist, 1));
	G = (top) ? calc_block_cone(goalside - ballpos + L, point(0, -1) + L, L, robot::MAX_RADIUS)
	: calc_block_cone(point(0, 1) + L, goalside - ballpos + L, L, robot::MAX_RADIUS);
	}
	 */
	{
		// distance on the goalside - ball line that the robot touches
		const point ball2side = goalside - ballpos;
		const point touchvec = -ball2side.norm(); // side to ball
		const double touchdist = std::min(-ball2side.x / 2, MAX_GOALIE_DIST);
		const point perp = (top) ? touchvec.rotate(-M_PI/2) : touchvec.rotate(M_PI/2); 
		G = goalside + touchvec * touchdist + perp * robot::MAX_RADIUS;
#warning a hack right now
		G.x = std::max(G.x, - f.length() / 2 + robot::MAX_RADIUS);
	}

	// first defender will block the remaining cone from the ball
	{
		point D1 = calc_block_cone_defender(goalside, goalopp, ballpos, G, robot::MAX_RADIUS);
		bool blowup = false;
		if (D1.x > robot::MAX_RADIUS - f.length() / 2) blowup = true;
		if (std::fabs(D1.y) > f.width() / 4) blowup = true;
		if (blowup) {
			D1 = (f.friendly_goal() + ballpos) / 2;
		}
		waypoints.push_back(D1);
	}

	// next two defenders block nearest enemy sights to goal if needed
	// enemies with ball possession are ignored (they should be handled above)
	for (size_t i = 0; i < enemies.size() && waypoints.size() < 3; ++i){
		if (!ai_util::ball_close(the_world, enemies[i])) {
			bool blowup = false;
			point D = calc_block_cone(goalside, goalopp, enemies[i]->position(), robot::MAX_RADIUS);
			if (D.x > robot::MAX_RADIUS - f.length() / 2) blowup = true;
			if (std::fabs(D.y) > f.width() / 4) blowup = true;
			if (blowup) {
				D = (f.friendly_goal() + enemies[i]->position()) / 2;
			}
			waypoints.push_back(D);
		}
	}

	// 4th defender go chase?
	waypoints.push_back(the_world->ball()->position());

	return std::make_pair(G, waypoints);
}

void defensive2::tick() {

	if (the_robots.size() == 0) {
		LOG_WARN("no robots");
		return;
	}

	const friendly_team& friendly(the_world->friendly);
	// const enemy_team& enemy(the_world->enemy);
	const point& ballpos = the_world->ball()->position();

	// the robot chaser
	double chaserdist = 1e99;
	player::ptr chaser;
	for (size_t i = 0; i < friendly.size(); ++i) {
		const double dist = (friendly[i]->position() - ballpos).len();
		if (dist > ai_util::CHASE_BALL_DIST) continue;
		if (!chaser || dist < chaserdist) {
			chaserdist = dist;
			chaser = friendly[i];
		}
	}

	if (chaser) LOG_INFO(Glib::ustring::compose("chaser is", chaser->name));

	/*
	   for (size_t i = 0; i < the_robots.size(); ++i) {
	   const double dist = (the_robots[i]->position() - ballpos).len();
	   if (dist > ai_util::CHASE_BALL_DIST) continue;
	   if (!chaser || dist < chaserdist) {
	   chaserdist = dist;
	   chaser = the_robots[i];
	   }
	   }
	 */

	// robot 0 is goalie, the others are non-goalie
	if (!goalie) {
		goalie = the_robots[0];
	} else {
		for (size_t i = 0; i < the_robots.size(); ++i) {
			if (the_robots[i] != goalie) continue;
			swap(the_robots[i], the_robots[0]);
			break;
		}
	}

	// adjust ball position
	defensive2_state::ptr state(defensive2_state::ptr::cast_dynamic(goalie->get_state(typeid(*this))));
	if (!state) {
		state = defensive2_state::ptr(new defensive2_state());
		goalie->set_state(typeid(*this), state);
	}
	if (ballpos.y > robot::MAX_RADIUS * 2) {
		state->top = true;
	} else if (ballpos.y < -robot::MAX_RADIUS * 2) {
		state->top = false;
	}

	std::vector<player::ptr> defenders;
	for (size_t i = 1; i < the_robots.size(); ++i)
		defenders.push_back(the_robots[i]);

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(defenders.begin(), defenders.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));
	std::vector<player::ptr> friends = ai_util::get_friends(friendly, defenders);

	const int baller = ai_util::calc_baller(the_world, defenders);
	// const bool teamball = ai_util::friendly_posses_ball(the_world);

	std::pair<point, std::vector<point> > positions = calc_block_positions(state->top);
	std::vector<point>& waypoints = positions.second;

	// do matching for distances

	std::vector<point> locations;
	for (size_t i = 0; i < defenders.size(); ++i) {
		locations.push_back(defenders[i]->position());
	}

	// ensure we are only blocking as we need
	while (waypoints.size() > defenders.size()) waypoints.pop_back();

	std::vector<size_t> order = dist_matching(locations, waypoints);

	// do the actual assigmment
	point ballvel = the_world->ball()->est_velocity();
	point rushpos = line_intersect(ballpos, ballpos + ballvel, 
					point(-the_world->field().length()/2.0 + robot::MAX_RADIUS, 1.0),
					point(-the_world->field().length()/2.0 + robot::MAX_RADIUS, -1.0));
	const bool goalierush = USE_GOALIE_RUSH && ballvel.x < -BALL_DANGEROUS_SPEED 
                              && (std::fabs(rushpos.y) < the_world->field().goal_width()/2.0);
	//const bool goaliechase = (chaser == goalie && ai_util::point_in_defense(the_world, ballpos));
	const bool goaliechase = ai_util::point_in_defense(the_world, ballpos);
	
	// check if goalie should rush
	if (goalierush){
		LOG_INFO("goalie to rush");
		move::ptr tactic(new move(the_robots[0], the_world));
		tactic->set_position(rushpos);
		tactics[0] = tactic;
	}
	// check if chaser robot
	if (goaliechase) {
		LOG_INFO("goalie to shoot");
		shoot::ptr shoot_tactic = shoot::ptr(new shoot(the_robots[0], the_world));
		shoot_tactic->force();
		shoot_tactic->set_pivot(false);
		tactics[0] = shoot_tactic;
	} else {
		move::ptr tactic(new move(the_robots[0], the_world));
		tactic->set_position(positions.first);
		tactics[0] = tactic;
		if (chaser == goalie && defenders.size() > 0) chaser = defenders[0];
	}

	size_t w = 0; // so we can skip robots as needed
	for (size_t i = 0; i < defenders.size(); ++i) {
		// if (static_cast<int>(i) == skipme) continue;
		if (w >= waypoints.size()) {
			LOG_WARN(Glib::ustring::compose("%1 nothing to do", defenders[i]->name));
			move::ptr tactic(new move(defenders[i], the_world));
			tactic->set_position(defenders[i]->position());
			assign(defenders[i], tactic);
			continue;
		} 

		// const point& target = waypoints[order[w]];
		if (chaser == defenders[i]) {
			// should be exact
			shoot::ptr shoot_tactic = shoot::ptr(new shoot(defenders[i], the_world));
			shoot_tactic->force();
			shoot_tactic->set_pivot(false);
			assign(defenders[i], shoot_tactic);
		} else {
			move::ptr tactic(new move(defenders[i], the_world));
			tactic->set_position(waypoints[order[w]]);
			assign(defenders[i], tactic);
		}
		++w;
	}

	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	unsigned int goalie_flags = flags & ~(ai_flags::avoid_friendly_defence|ai_flags::avoid_ball_stop);
	if (baller == 0) {
		tactics[0]->set_flags(goalie_flags | ai_flags::clip_play_area);
	} else {
		tactics[0]->set_flags(goalie_flags);
	}
	tactics[0]->tick();
	for (size_t i = 1; i < tactics.size(); ++i) {
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

