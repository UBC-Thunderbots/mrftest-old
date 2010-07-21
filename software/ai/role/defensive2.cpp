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
	DoubleParam MIN_GOALPOST_DIST("defensive2: distance to goal post", 0.03, 0.03, 1.0);
	DoubleParam MAX_GOALIE_DIST("defensive2: goalie dist (robot radius)", 2.0, 0.0, 10.0);

	BoolParam USE_GOALIE_RUSH("defensive2: use goalie rush when ball threatening", TRUE);
	DoubleParam BALL_DANGEROUS_SPEED("defensive2: threatening ball speed", 0.1, 0.1, 10.0); 
	DoubleParam DEFENSIVE2_SHRINK("defensive2: shrink robot radius", 0.9, 0.0, 2.0);
	DoubleParam DEFENSIVE_DIST("defensive2: goalie will block center of goal when defender farther than this distance", 0.5, 0.1, 10.0);

	// used to save if the goalie should be on the top or bottom side
	class Defensive2State : public Player::State {
		public:
			typedef RefPtr<Defensive2State> Ptr;
			Defensive2State() : top(false) { }
			bool top;
	};

}

Defensive2::Defensive2(World::Ptr world) : the_world(world) {
}

void Defensive2::assign(const Player::Ptr& p, Tactic::Ptr t) {
	for (size_t i = 0; i < players.size(); ++i) {
		if (players[i] == p) {
			tactics[i] = t;
			return;
		}
	}
	LOG_ERROR("assign unknown robot");
}

std::pair<Point, std::vector<Point> > Defensive2::calc_block_positions(const bool top) const {
	const EnemyTeam& enemy(the_world->enemy);

	const Field& f = the_world->field();

	// Sort enemies by distance from own goal.
	std::vector<Robot::Ptr> enemies = enemy.get_robots();
	//std::sort(enemies.begin(), enemies.end(), AIUtil::CmpDist<Robot::Ptr>(the_world->ball()->position()));
	std::sort(enemies.begin(), enemies.end(), AIUtil::CmpDist<Robot::Ptr>(f.friendly_goal()));

	std::vector<Point> waypoints;

	// there is cone ball to goal sides, bounded by 1 rays.
	const Point& ballpos = the_world->ball()->position();
	const Point goalside = top ? Point(-f.length()/2, f.goal_width()/2) : Point(-f.length()/2, -f.goal_width()/2);
	const Point goalopp = top ? Point(-f.length()/2, -f.goal_width()/2) : Point(-f.length()/2, f.goal_width()/2);

	const double radius = Robot::MAX_RADIUS * DEFENSIVE2_SHRINK;

	Point G;
	/*
	   {
	// goalie and first defender integrated defence
	// maximum x-distance the goalie can go from own goal.
	// normally
	const double maxdist = f.defense_area_radius();
	const Point L = line_intersect(goalside, ballpos, f.friendly_goal() + Point(maxdist, -1), f.friendly_goal() + Point(maxdist, 1));
	G = (top) ? calc_block_cone(goalside - ballpos + L, Point(0, -1) + L, L, shrink)
	: calc_block_cone(Point(0, 1) + L, goalside - ballpos + L, L, shrink);
	}
	 */
	{
		// distance on the goalside - ball line that the robot touches
		const Point ball2side = goalside - ballpos;
		const Point touchvec = -ball2side.norm(); // side to ball
		const double touchdist = std::min(-ball2side.x / 2, MAX_GOALIE_DIST * Robot::MAX_RADIUS);
		const Point perp = (top) ? touchvec.rotate(-M_PI/2) : touchvec.rotate(M_PI/2); 
		G = goalside + touchvec * touchdist + perp * radius;
#warning a hack right now
		G.x = std::max(G.x, - f.length() / 2 + radius);
	}

	// first defender will block the remaining cone from the ball
	{
		Point D1 = calc_block_cone_defender(goalside, goalopp, ballpos, G, radius);
		bool blowup = false;
		if (D1.x < Robot::MAX_RADIUS - f.length() / 2 + f.defense_area_stretch()) blowup = true;
		if (std::fabs(D1.y) > f.width() / 4) blowup = true;
		if (blowup) {
			D1 = (f.friendly_goal() + ballpos) / 2;
		}
		waypoints.push_back(D1);
	}

	// next two defenders block nearest enemy sights to goal if needed
	// enemies with ball possession are ignored (they should be handled above)
	for (size_t i = 0; i < enemies.size() && waypoints.size() < 3; ++i){
		if (!AIUtil::ball_close(the_world, enemies[i])) {
			bool blowup = false;
			Point D = calc_block_cone(goalside, goalopp, enemies[i]->position(), radius);
			if (D.x < Robot::MAX_RADIUS - f.length() / 2 + f.defense_area_stretch()) blowup = true;
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

void Defensive2::tick() {

	tactics.clear();
	tactics.resize(players.size());

	if (players.size() == 0) {
		LOG_WARN("no robots");
		return;
	}

	const FriendlyTeam& friendly(the_world->friendly);
	// const EnemyTeam& enemy(the_world->enemy);
	const Point& ballpos = the_world->ball()->position();

	// the robot chaser
	double chaserdist = 1e99;
	Player::Ptr chaser;
	for (size_t i = 0; i < friendly.size(); ++i) {
		const double dist = (friendly[i]->position() - ballpos).len();
		if (dist > AIUtil::CHASE_BALL_DIST) continue;
		if (!chaser || dist < chaserdist) {
			chaserdist = dist;
			chaser = friendly[i];
		}
	}

	if (chaser) LOG_INFO(Glib::ustring::compose("chaser is", chaser->name));

	/*
	   for (size_t i = 0; i < robots.size(); ++i) {
	   const double dist = (robots[i]->position() - ballpos).len();
	   if (dist > AIUtil::CHASE_BALL_DIST) continue;
	   if (!chaser || dist < chaserdist) {
	   chaserdist = dist;
	   chaser = robots[i];
	   }
	   }
	 */

	// robot 0 is goalie, the others are non-goalie
	if (!goalie) {
		goalie = players[0];
	} else {
		for (size_t i = 0; i < players.size(); ++i) {
			if (players[i] != goalie) continue;
			swap(players[i], players[0]);
			break;
		}
	}

	// adjust ball position
	Defensive2State::Ptr state(Defensive2State::Ptr::cast_dynamic(goalie->get_state(typeid(*this))));
	if (!state) {
		state = Defensive2State::Ptr(new Defensive2State());
		goalie->set_state(typeid(*this), state);
	}
	if (ballpos.y > Robot::MAX_RADIUS * 2) {
		state->top = true;
	} else if (ballpos.y < -Robot::MAX_RADIUS * 2) {
		state->top = false;
	}

	std::vector<Player::Ptr> defenders;
	for (size_t i = 1; i < players.size(); ++i)
		defenders.push_back(players[i]);

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(defenders.begin(), defenders.end(), AIUtil::CmpDist<Player::Ptr>(the_world->ball()->position()));
	std::vector<Player::Ptr> friends = AIUtil::get_friends(friendly, defenders);

	const int baller = AIUtil::calc_baller(the_world, defenders);
	// const bool teamball = AIUtil::friendly_posses_ball(the_world);

	std::pair<Point, std::vector<Point> > positions = calc_block_positions(state->top);
	std::vector<Point>& waypoints = positions.second;

	// do matching for distances

	std::vector<Point> locations;
	for (size_t i = 0; i < defenders.size(); ++i) {
		locations.push_back(defenders[i]->position());
	}

	// ensure we are only blocking as we need
	while (waypoints.size() > defenders.size()) waypoints.pop_back();

	std::vector<size_t> order = dist_matching(locations, waypoints);

	// do the actual assigmment
	Point ballvel = the_world->ball()->est_velocity();
	Point rushpos, goalpos;
	if (ballvel.len() > BALL_DANGEROUS_SPEED && ballvel.x < -1e-6){
		rushpos = line_intersect(ballpos, ballpos + ballvel, 
					Point(-the_world->field().length()/2.0 + 1.5*Robot::MAX_RADIUS, 1.0),
					Point(-the_world->field().length()/2.0 + 1.5*Robot::MAX_RADIUS, -1.0));

		goalpos = line_intersect(ballpos, ballpos + ballvel, 
					Point(-the_world->field().length()/2.0, 1.0),
					Point(-the_world->field().length()/2.0, -1.0));
		LOG_INFO(Glib::ustring::compose("ball heading towards our side of the field: rushpos.y = %1, goalpos.y = %2", rushpos.y, goalpos.y));
	}
	const bool goalierush = USE_GOALIE_RUSH && ballvel.len() > BALL_DANGEROUS_SPEED && ballvel.x < -1e-6 
                              && (std::min(std::fabs(goalpos.y),std::fabs(rushpos.y)) < the_world->field().goal_width()/2.0);
	//const bool goaliechase = (chaser == goalie && AIUtil::point_in_defense(the_world, ballpos));
	const bool goaliechase = AIUtil::point_in_defense(the_world, ballpos);
	
	// check if goalie should rush
	if (goalierush){
		LOG_INFO("goalie to rush");
		Move::Ptr tactic(new Move(players[0], the_world));
		rushpos.y = std::min(rushpos.y, the_world->field().goal_width()/2.0);
		rushpos.y = std::max(rushpos.y, -the_world->field().goal_width()/2.0);
		tactic->set_position(rushpos);
		tactics[0] = tactic;
	}
	// check if chaser robot
	else if (goaliechase) {
		LOG_INFO("goalie to shoot");
		Shoot::Ptr shoot_tactic = Shoot::Ptr(new Shoot(players[0], the_world));
		shoot_tactic->force();
		shoot_tactic->set_pivot(false);
		tactics[0] = shoot_tactic;
	} else {
		Move::Ptr tactic(new Move(players[0], the_world));
		int closest_defender = - 1;
		for (size_t i = 0; i < defenders.size(); ++i)
			if (order[i] == 0)
				closest_defender = static_cast<int>(i);
		if (closest_defender != -1 && (defenders[closest_defender]->position() - waypoints[0]).len() > DEFENSIVE_DIST)
			tactic->set_position(Point(-the_world->field().length()/2.0+1.5*Robot::MAX_RADIUS, 0));
		else tactic->set_position(positions.first);
		tactics[0] = tactic;
		if (chaser == goalie && defenders.size() > 0) chaser = defenders[0];
	}

	size_t w = 0; // so we can skip robots as needed
	for (size_t i = 0; i < defenders.size(); ++i) {
		// if (static_cast<int>(i) == skipme) continue;
		if (w >= waypoints.size()) {
			LOG_WARN(Glib::ustring::compose("%1 nothing to do", defenders[i]->name));
			Move::Ptr tactic(new Move(defenders[i], the_world));
			tactic->set_position(defenders[i]->position());
			assign(defenders[i], tactic);
			continue;
		} 

		// const Point& target = waypoints[order[w]];
		if (chaser == defenders[i]) {
			// should be exact
			Shoot::Ptr shoot_tactic = Shoot::Ptr(new Shoot(defenders[i], the_world));
			shoot_tactic->force();
			shoot_tactic->set_pivot(false);
			assign(defenders[i], shoot_tactic);
		} else {
			Move::Ptr tactic(new Move(defenders[i], the_world));
			tactic->set_position(waypoints[order[w]]);
			assign(defenders[i], tactic);
		}
		++w;
	}

	unsigned int flags = AIFlags::calc_flags(the_world->playtype());
	unsigned int goalie_flags = flags & ~(AIFlags::AVOID_FRIENDLY_DEFENSE|AIFlags::AVOID_BALL_STOP);
	if (baller == 0) {
		tactics[0]->set_flags(goalie_flags | AIFlags::CLIP_PLAY_AREA);
	} else {
		tactics[0]->set_flags(goalie_flags);
	}
	tactics[0]->tick();
	for (size_t i = 1; i < tactics.size(); ++i) {
		if (static_cast<int>(i) == baller) {
			tactics[i]->set_flags(flags | AIFlags::CLIP_PLAY_AREA);
		} else {
			tactics[i]->set_flags(flags);
		}
		tactics[i]->tick();
	}
}

void Defensive2::players_changed() {
}
