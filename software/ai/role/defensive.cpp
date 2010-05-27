#include "ai/role/defensive.h"
#include "ai/role/goalie.h"
#include "ai/tactic/chase.h"
#include "ai/tactic/move.h"
#include "ai/tactic/pass.h"
#include "ai/tactic/receive.h"
#include "ai/util.h"
#include "util/algorithm.h"

defensive::defensive(world::ptr world) : the_world(world) {
}

// TODO: This function is obselete.
void defensive::move_halfway_between_ball_and_our_goal(int index) {
	const field &the_field(the_world->field());
	move::ptr tactic( new move(the_robots[index], the_world));
	double x_pos  = -1*the_field.length()/2 + (the_field.length()/2 + the_world->ball()->position().x) /2;
	double y_pos = the_robots[index]->position().y;
	tactic->set_position(point(x_pos, y_pos));
	the_tactics.push_back(tactic);
}

// TODO: This function is obselete.
void defensive::tick_goalie() {
	if (the_goalie == NULL) return;

	if (the_goalie->has_ball()) {
		if (the_robots.size()==0) { // there is no one to pass to
			//TODO the goalie is the only robot in the field, it should probably kick the ball to the other side of the field ASAP...but this is up to you. 
		} else {
			//TODO decide whether you want the goalie to pass, or do something else...
			//You can set it to use any tactic and tick it. (you can do that anywhere if you like as long as you don't set it to a goalie role)
		}
		// if you still want the goalie to to use the goalie role, just copy the following code.
	} else {
		// if the goalie doesn't have ball, it should act like a goalie.
		goalie::ptr temp_role(new goalie(the_world));
		goalie_role = temp_role;
		std::vector<player::ptr> goalie_only;
		goalie_only.push_back(the_goalie);
		goalie_role->set_robots(goalie_only);
		goalie_role->tick();
	}
}

std::vector<point> defensive::calc_block_positions() const {
	const enemy_team& enemy(the_world->enemy);

	// Sort enemies by distance from own goal.
	std::vector<robot::ptr> enemies = enemy.get_robots();
	std::sort(enemies.begin(), enemies.end(), ai_util::cmp_dist<robot::ptr>(the_world->field().friendly_goal()));

	// Place waypoints on the defence area.
	// TODO: calculate proper areas in the future.
	std::vector<point> waypoints;
	for (size_t i = 0; i < enemies.size(); ++i) {
		point half = (enemies[i]->position() + the_world->field().friendly_goal()) * 0.5;
		waypoints.push_back(half);
	}

	// TODO: have only up to one defensive robot in the defence area.

	return waypoints;
}

void defensive::tick() {

	the_tactics.clear();

	// TODO: remove in the future.
	tick_goalie();

	if (the_robots.size() == 0) return;

	// Sort by distance to ball.
	// DO NOT SORT IT AGAIN!!
	std::sort(the_robots.begin(), the_robots.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));

	const friendly_team& friendly(the_world->friendly);
	
	int baller = -1;
	for(size_t i = 0; i < the_robots.size(); i++) {
		if(the_robots[i]->has_ball()) {
			baller = i;
			break;
		}
	}

	bool teamhasball = false;
	for(size_t i = 0; i < friendly.size(); i++) {
		if(friendly.get_player(i)->has_ball()) {
			teamhasball = true;
			break;
		}
	}
	  
	std::vector<player::ptr> friends = ai_util::get_friends(friendly, the_robots);

	// The robot that will do something to the ball (e.g. chase).
	// Other robots will just go defend or something.
	int busyidx = -1;

	if (teamhasball) {
		if (baller >= 0) {
			// If a player in the role has a ball, then
			// pass to the other friendly, or wait if there is none.
			std::sort(friends.begin(), friends.end(), ai_util::cmp_dist<player::ptr>(the_world->field().enemy_goal()));
			int nearidx = -1;
			for (size_t i = 0; i < friends.size(); ++i) {
				if (!ai_util::can_pass(the_world, friends[i])) continue;
				nearidx = i;
				break;
			}

			// TODO: do something
			if (nearidx == -1) {
				// ehh... nobody to pass to
				// Just play around with the ball I guess
				move::ptr move_tactic(new move(the_robots[baller], the_world));
				move_tactic->set_position(the_robots[baller]->position());
				the_tactics.push_back(move_tactic);
			} else {
				pass::ptr pass_tactic(new pass(the_robots[baller], the_world, friends[nearidx]));
				the_tactics.push_back(pass_tactic);
			}

			busyidx = baller;
		} else {
			// If a player nearest to the goal area has the ball
			// that player is probably a goalie, chase the ball!
			std::sort(friends.begin(), friends.end(), ai_util::cmp_dist<player::ptr>(the_world->field().friendly_goal()));
			if (friends.size() > 0 && friends[0]->has_ball()) {
				receive::ptr receive_tactic(new receive(the_robots[0], the_world));
				the_tactics.push_back(receive_tactic);
				busyidx = 0;
			}
		}
	} else {
		// already sorted by distance to ball
		chase::ptr chase_tactic(new chase(the_robots[0], the_world));
		the_tactics.push_back(chase_tactic);
		busyidx = 0;
	}

	std::vector<point> waypoints = calc_block_positions();

	std::vector<player::ptr> available;
	std::vector<point> locations;
	for (size_t i = 0; i < the_robots.size(); ++i) {
		if (static_cast<int>(i) == busyidx) continue;
		available.push_back(the_robots[i]);
		locations.push_back(the_robots[i]->position());
	}

	std::vector<size_t> order = ai_util::dist_matching(locations, waypoints);

	size_t w = 0;
	for (size_t i = 0; i < waypoints.size(); ++i) {
		if (w >= waypoints.size()) {
			// std::cerr << "Defender has nothing to do!" << std::endl;
			move::ptr move_tactic(new move(the_robots[i], the_world));
			move_tactic->set_position(the_robots[i]->position());
			the_tactics.push_back(move_tactic);
		} else {
			move::ptr move_tactic(new move(the_robots[i], the_world));
			move_tactic->set_position(waypoints[order[i]]);
			the_tactics.push_back(move_tactic);
			w++;
		}
	}

	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	if (teamhasball) 
	  flags |= ai_flags::clip_play_area;

	for (size_t i = 0; i < the_tactics.size(); i++) {
		the_tactics[i]->set_flags(flags);
		the_tactics[i]->tick();
	}
}

void defensive::robots_changed() {
	tick();
}

void defensive::set_goalie(const player::ptr goalie) {
	the_goalie = goalie;
}

