#include "ai/role/defensive3.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/pass.h"
#include "ai/util.h"
#include "ai/flags.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "uicomponents/param.h"

#include <iostream>
#include <map>

namespace {
	// minimum distance from the goal post
	DoubleParam MIN_GOALPOST_DIST("defensive3: distance to goal post", 0.03, 0.03, 1.0);
	DoubleParam MAX_GOALIE_DIST("defensive3: goalie dist (robot radius)", 2.0, 0.0, 10.0);

	BoolParam USE_GOALIE_RUSH("defensive3: use goalie rush when ball threatening", TRUE);
	DoubleParam BALL_DANGEROUS_SPEED("defensive3: threatening ball speed", 0.1, 0.1, 10.0); 
	DoubleParam DEFENSIVE2_SHRINK("defensive3: shrink robot radius", 0.9, 0.0, 2.0);
	DoubleParam DEFENSIVE_DIST("defensive3: goalie will block center of goal when defender farther than this distance", 0.5, 0.1, 10.0);

}

Defensive3::Defensive3(const World::ptr world) : world(world) {
	// goalie_guard_top initialization is optional
}

std::pair<Point, std::vector<Point> > Defensive3::calc_block_positions(const bool top) const {
	const EnemyTeam& enemy(world->enemy);

	const Field& f = world->field();

	// Sort enemies by distance from own goal.
	std::vector<Robot::ptr> enemies = enemy.get_robots();
	std::sort(enemies.begin(), enemies.end(), AIUtil::CmpDist<Robot::ptr>(f.friendly_goal()));

	std::vector<Point> waypoints;

	// there is cone ball to goal sides, bounded by 1 rays.
	const Point& ballpos = world->ball()->position();
	const Point goalside = top ? Point(-f.length()/2, f.goal_width()/2) : Point(-f.length()/2, -f.goal_width()/2);
	const Point goalopp = top ? Point(-f.length()/2, -f.goal_width()/2) : Point(-f.length()/2, f.goal_width()/2);

	const double radius = Robot::MAX_RADIUS * DEFENSIVE2_SHRINK;

	Point G;
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
		if (!AIUtil::ball_close(world, enemies[i])) {
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
	waypoints.push_back(world->ball()->position());

	return std::make_pair(G, waypoints);
}

void Defensive3::tick() {

	// stateful ai can have empty roles
	if (players.size() == 0) return;
	std::map<Player::ptr, Tactic::ptr> tactics;

	const FriendlyTeam& friendly(world->friendly);

	const Point& ballpos = world->ball()->position();

	// the robot chaser
	double chaserdist = 1e99;
	Player::ptr chaser;
	for (size_t i = 0; i < friendly.size(); ++i) {
		const double dist = (friendly[i]->position() - ballpos).len();
		if (dist > AIUtil::CHASE_BALL_DIST) continue;
		if (!chaser || dist < chaserdist) {
			chaserdist = dist;
			chaser = friendly[i];
		}
	}

	if (chaser) {
		LOG_INFO(Glib::ustring::compose("chaser is", chaser->name));
	}

	// robot 0 is goalie, the others are non-goalie
	if (!goalie) {
		goalie = *players.begin();
		LOG_ERROR("goalie not set");
	}

	if (ballpos.y > Robot::MAX_RADIUS * 2) {
		goalie_guard_top = true;
	} else if (ballpos.y < -Robot::MAX_RADIUS * 2) {
		goalie_guard_top = false;
	}

	Player::ptr baller;
	std::vector<Player::ptr> defenders;
	for (std::set<Player::ptr>::iterator it = players.begin(); it != players.end(); ++it) {
#warning more than 1 player may posses ball
		if (AIUtil::posses_ball(world, *it)) baller = *it;
		if (*it == goalie) continue;
		defenders.push_back(*it);
	}

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(defenders.begin(), defenders.end(), AIUtil::CmpDist<Player::ptr>(world->ball()->position()));

	std::pair<Point, std::vector<Point> > positions = calc_block_positions(goalie_guard_top);
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
	Point ballvel = world->ball()->est_velocity();
	Point rushpos, goalpos;
	if (ballvel.len() > BALL_DANGEROUS_SPEED && ballvel.x < -1e-6){
		rushpos = line_intersect(ballpos, ballpos + ballvel, 
				Point(-world->field().length()/2.0 + 1.5*Robot::MAX_RADIUS, 1.0),
				Point(-world->field().length()/2.0 + 1.5*Robot::MAX_RADIUS, -1.0));

		goalpos = line_intersect(ballpos, ballpos + ballvel, 
				Point(-world->field().length()/2.0, 1.0),
				Point(-world->field().length()/2.0, -1.0));
		LOG_INFO(Glib::ustring::compose("ball heading towards our side of the field: rushpos.y = %1, goalpos.y = %2", rushpos.y, goalpos.y));
	}
	const bool goalierush = USE_GOALIE_RUSH && ballvel.len() > BALL_DANGEROUS_SPEED && ballvel.x < -1e-6 
		&& (std::min(std::fabs(goalpos.y),std::fabs(rushpos.y)) < world->field().goal_width()/2.0);
	//const bool goaliechase = (chaser == goalie && AIUtil::point_in_defense(world, ballpos));
	const bool goaliechase = AIUtil::point_in_defense(world, ballpos);

#warning goalie should defend the middle if no other defenders exist
	if (goalierush){ // check if goalie should rush
		LOG_INFO("goalie to rush");
		Move::ptr tactic(new Move(goalie, world));
		rushpos.y = std::min(rushpos.y, world->field().goal_width()/2.0);
		rushpos.y = std::max(rushpos.y, -world->field().goal_width()/2.0);
		tactic->set_position(rushpos);
		tactics[goalie] = tactic;
	} else if (goaliechase) { // check if chaser robot
		LOG_INFO("goalie to shoot");
		Shoot::ptr shoot_tactic = Shoot::ptr(new Shoot(goalie, world));
		shoot_tactic->force();
		shoot_tactic->set_pivot(false);
		tactics[goalie] = shoot_tactic;
	} else {
		Move::ptr tactic(new Move(goalie, world));
		int closest_defender = - 1;
		for (size_t i = 0; i < defenders.size(); ++i) {
			if (order[i] == 0) {
				closest_defender = static_cast<int>(i);
			}
		}
		if (closest_defender != -1 && (defenders[closest_defender]->position() - waypoints[0]).len() > DEFENSIVE_DIST)
			tactic->set_position(Point(-world->field().length()/2.0+1.5*Robot::MAX_RADIUS, 0));
		else tactic->set_position(positions.first);
		tactics[goalie] = tactic;
		if (chaser == goalie && defenders.size() > 0) chaser = defenders[0];
	}

	size_t w = 0; // so we can skip players as needed
	for (size_t i = 0; i < defenders.size(); ++i) {
		if (chaser == defenders[i]) { // should be exact
			Shoot::ptr shoot_tactic = Shoot::ptr(new Shoot(defenders[i], world));
			shoot_tactic->force();
			shoot_tactic->set_pivot(false);
			tactics[defenders[i]] = shoot_tactic;
		} else if (w >= waypoints.size()) {
			LOG_WARN(Glib::ustring::compose("%1 nothing to do", defenders[i]->name));
			Move::ptr tactic(new Move(defenders[i], world));
			tactic->set_position(defenders[i]->position());
			tactics[defenders[i]] = tactic;
		} else {
			Move::ptr tactic(new Move(defenders[i], world));
			tactic->set_position(waypoints[order[w]]);
			tactics[defenders[i]] = tactic;
			++w;
		}
	}

	// set flags
	unsigned int flags = AIFlags::calc_flags(world->playtype());
	unsigned int goalie_flags = flags & ~(AIFlags::AVOID_FRIENDLY_DEFENSE|AIFlags::AVOID_BALL_STOP);
	if (baller == goalie) {
		tactics[goalie]->set_flags(goalie_flags | AIFlags::CLIP_PLAY_AREA);
	} else {
		tactics[goalie]->set_flags(goalie_flags);
	}
	tactics[goalie]->tick();
	for (size_t i = 0; i < defenders.size(); ++i) {
		if (defenders[i] == baller) {
			tactics[defenders[i]]->set_flags(flags | AIFlags::CLIP_PLAY_AREA);
		} else {
			tactics[defenders[i]]->set_flags(flags);
		}
		tactics[defenders[i]]->tick();
	}
}

void Defensive3::add_player(Player::ptr player) {
	if (!goalie) {
		if (players.size() != 0) {
			LOG_ERROR("player count inconsistency");
		}
		goalie = player;
	}
	if (players.find(player) != players.end()) {
		LOG_ERROR("player already exist");
	}
	players.insert(player);
}

void Defensive3::remove_player(Player::ptr player) {
	if (players.find(player) == players.end()) {
		LOG_ERROR("attempting to remove nonexistent player");
		return;
	}
	players.erase(player);
	if (player == goalie) {
		if (players.size() > 0) {
			goalie = *players.begin();
		} else {
			goalie.clear();
		}
	}
}

void Defensive3::clear_players() {
	players.clear();
	goalie.clear();
}

void Defensive3::set_goalie(Player::ptr player) {
	goalie = player;
	players.insert(goalie);
}

Player::ptr Defensive3::pop_player() {
	Player::ptr player;
	if (players.size() == 0) {
		LOG_ERROR("role has no robots");
		return player;
	}
	if (players.size() == 1) { // no choice, goalie is gone
		player = *players.begin();
		players.clear();
		goalie.clear();
		return player;
	}
#warning TODO find better mechanism
	for (std::set<Player::ptr>::iterator it = players.begin(); it != players.end(); ++it) {
		if (goalie == *it) continue;
		player = *it;
		break;
	}
	players.erase(player);
	return player;
}

