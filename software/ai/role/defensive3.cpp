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

Defensive3::Defensive3(const World::Ptr world) : world(world) {
	// goalie_guard_top initialization is optional
}

void Defensive3::calc_block_positions() {
	const EnemyTeam& enemy(world->enemy);

	const Field& f = world->field();

	// Sort enemies by distance from own goal.
	std::vector<Robot::Ptr> enemies = enemy.get_robots();
	std::sort(enemies.begin(), enemies.end(), AIUtil::CmpDist<Robot::Ptr>(f.friendly_goal()));

	defender_positions.clear();

	// there is cone ball to goal sides, bounded by 1 rays.
	const Point& ballpos = world->ball()->position();
	const Point goalside = goalie_guard_top ? Point(-f.length()/2, f.goal_width()/2) : Point(-f.length()/2, -f.goal_width()/2);
	const Point goalopp = goalie_guard_top ? Point(-f.length()/2, -f.goal_width()/2) : Point(-f.length()/2, f.goal_width()/2);

	const double radius = Robot::MAX_RADIUS * DEFENSIVE2_SHRINK;

	{
		// distance on the goalside - ball line that the robot touches
		const Point ball2side = goalside - ballpos;
		const Point touchvec = -ball2side.norm(); // side to ball
		const double touchdist = std::min(-ball2side.x / 2, MAX_GOALIE_DIST * Robot::MAX_RADIUS);
		const Point perp = (goalie_guard_top) ? touchvec.rotate(-M_PI/2) : touchvec.rotate(M_PI/2); 
		goalie_position = goalside + touchvec * touchdist + perp * radius;
#warning a hack right now
		goalie_position.x = std::max(goalie_position.x, - f.length() / 2 + radius);
	}

	// first defender will block the remaining cone from the ball
	{
		Point D1 = calc_block_cone_defender(goalside, goalopp, ballpos, goalie_position, radius);
		bool blowup = false;
		if (D1.x < Robot::MAX_RADIUS - f.length() / 2 + f.defense_area_stretch()) blowup = true;
		if (std::fabs(D1.y) > f.width() / 4) blowup = true;
		if (blowup) {
			D1 = (f.friendly_goal() + ballpos) / 2;
		}
		defender_positions.push_back(D1);
	}

	// next two defenders block nearest enemy sights to goal if needed
	// enemies with ball possession are ignored (they should be handled above)
	for (size_t i = 0; i < enemies.size() && defender_positions.size() < 3; ++i){
		if (!AIUtil::ball_close(world, enemies[i])) {
			bool blowup = false;
			Point D = calc_block_cone(goalside, goalopp, enemies[i]->position(), radius);
			if (D.x < Robot::MAX_RADIUS - f.length() / 2 + f.defense_area_stretch()) blowup = true;
			if (std::fabs(D.y) > f.width() / 4) blowup = true;
			if (blowup) {
				D = (f.friendly_goal() + enemies[i]->position()) / 2;
			}
			defender_positions.push_back(D);
		}
	}

	// 4th defender go chase?
	defender_positions.push_back(world->ball()->position());
}

void Defensive3::tick() {

	// stateful ai can have empty roles
	if (players.size() == 0) return;
	std::map<Player::Ptr, Tactic::Ptr> tactics;

	const FriendlyTeam& friendly(world->friendly);

	const Point& ballpos = world->ball()->position();

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

	Player::Ptr baller;
	std::vector<Player::Ptr> defenders;
	for (std::set<Player::Ptr>::iterator it = players.begin(); it != players.end(); ++it) {
#warning more than 1 player may posses ball
		if (AIUtil::posses_ball(world, *it)) baller = *it;
		if (*it == goalie) continue;
		defenders.push_back(*it);
	}

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(defenders.begin(), defenders.end(), AIUtil::CmpDist<Player::Ptr>(world->ball()->position()));

	calc_block_positions();

	// do matching for distances

	std::vector<Point> locations;
	for (size_t i = 0; i < defenders.size(); ++i) {
		locations.push_back(defenders[i]->position());
	}

	// ensure we are only blocking as we need
	while (defender_positions.size() > defenders.size()) defender_positions.pop_back();

	std::vector<size_t> order = dist_matching(locations, defender_positions);

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
		Move::Ptr tactic(new Move(goalie, world));
		rushpos.y = std::min(rushpos.y, world->field().goal_width()/2.0);
		rushpos.y = std::max(rushpos.y, -world->field().goal_width()/2.0);
		tactic->set_position(rushpos);
		tactics[goalie] = tactic;
	} else if (goaliechase) { // check if chaser robot
		LOG_INFO("goalie to shoot");
		Shoot::Ptr shoot_tactic = Shoot::Ptr(new Shoot(goalie, world));
		shoot_tactic->force();
		shoot_tactic->set_pivot(false);
		tactics[goalie] = shoot_tactic;
	} else {
		Move::Ptr tactic(new Move(goalie, world));
		int closest_defender = - 1;
		for (size_t i = 0; i < defenders.size(); ++i) {
			if (order[i] == 0) {
				closest_defender = static_cast<int>(i);
			}
		}
		if (closest_defender != -1 && (defenders[closest_defender]->position() - defender_positions[0]).len() > DEFENSIVE_DIST)
			tactic->set_position(Point(-world->field().length()/2.0+1.5*Robot::MAX_RADIUS, 0));
		else tactic->set_position(goalie_position);
		tactics[goalie] = tactic;
		if (chaser == goalie && defenders.size() > 0) chaser = defenders[0];
	}

	size_t w = 0; // so we can skip players as needed
	for (size_t i = 0; i < defenders.size(); ++i) {
		if (chaser == defenders[i]) { // should be exact
			Shoot::Ptr shoot_tactic = Shoot::Ptr(new Shoot(defenders[i], world));
			shoot_tactic->force();
			shoot_tactic->set_pivot(false);
			tactics[defenders[i]] = shoot_tactic;
		} else if (w >= defender_positions.size()) {
			LOG_WARN(Glib::ustring::compose("%1 nothing to do", defenders[i]->name));
			Move::Ptr tactic(new Move(defenders[i], world));
			tactic->set_position(defenders[i]->position());
			tactics[defenders[i]] = tactic;
		} else {
			Move::Ptr tactic(new Move(defenders[i], world));
			tactic->set_position(defender_positions[order[w]]);
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

void Defensive3::add_player(Player::Ptr player) {
	if (!player) {
		LOG_ERROR("null player");
		return;
	}
	// maybe move setting goalie to tick?
	if (!goalie) {
		if (players.size() != 0) {
			LOG_ERROR("player count inconsistency");
		}
		goalie = player;
	}
	if (players.find(player) != players.end()) {
		LOG_WARN("player already exist");
	}
	players.insert(player);
}

void Defensive3::remove_player(Player::Ptr player) {
	if (!player) {
		LOG_ERROR("null player");
		return;
	}
	if (players.find(player) == players.end()) {
		LOG_ERROR("attempting to remove nonexistent player");
		return;
	}
	players.erase(player);
	// maybe move setting goalie to tick?
	if (player == goalie) {
		if (players.size() > 0) {
			goalie = *players.begin();
		} else {
			goalie.clear();
		}
	}
}

void Defensive3::clear_players() {
	const FriendlyTeam& friendly(world->friendly);
	if (friendly.size() > 1) { // if there is only one player, can't be helped
		switch (world->playtype()) {
			case PlayType::EXECUTE_PENALTY_ENEMY:
			case PlayType::PREPARE_PENALTY_ENEMY:
			case PlayType::PIT_STOP:
			case PlayType::VICTORY_DANCE:
				break;
			default:
				LOG_ERROR("clearing players is not intended for this playtype");
		}
	}
	players.clear();
	goalie.clear();
}

void Defensive3::set_goalie(Player::Ptr player) {
	if (!player) {
		LOG_ERROR("null player");
		return;
	}
	goalie = player;
	players.insert(goalie);
}

Player::Ptr Defensive3::pop_player() {
	Player::Ptr player;
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
	for (std::set<Player::Ptr>::iterator it = players.begin(); it != players.end(); ++it) {
		if (goalie == *it) continue;
		player = *it;
		break;
	}
	players.erase(player);
	return player;
}

void Defensive3::set_chaser(Player::Ptr player) {
#warning fix
	if (!player) {
		LOG_INFO("nobody chase the ball?");
		return;
	}
	// do something
}

void Defensive3::deprecated_set_players(std::vector<Player::Ptr>& ps) {
	goalie.clear();
	players.clear();
	if (ps.size() == 0) return;
	goalie = ps[0];
	for (size_t i = 0; i < ps.size(); ++i) {
		players.insert(ps[i]);
	}
}

