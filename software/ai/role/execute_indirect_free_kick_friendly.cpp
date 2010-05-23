#include "ai/role/execute_indirect_free_kick_friendly.h"
#include "ai/util.h"
#include <vector>

execute_indirect_free_kick_friendly::execute_indirect_free_kick_friendly(world::ptr world) : the_world(world) {
}

void execute_indirect_free_kick_friendly::tick(){
	const friendly_team &f_team(the_world->friendly);
	// Sort players by their distance to our goalpost.
	// Don't pass to closest player to our goal.
	std::vector<player::ptr> the_team = f_team.get_players();
	std::sort(the_team.begin(),the_team.end(), ai_util::cmp_dist<player::ptr>(the_world->field().friendly_goal()));
	unsigned int best_passee = the_team.size();
	double best_dist = 1e99;
	// Check for best robot to pass to.
	// NEVER pass to goalie, presumably closest goal to our goalpost (in case we get an own goal).
	for (unsigned int i = 1; i < the_team.size(); i++){
		if (the_team[i] == the_robots[0]) continue;
		if (ai_util::can_pass(the_world,the_team[i])){ 
			double new_dist = (the_team[i]->position()-the_robots[0]->position()).len();
			if (best_dist > new_dist){
				best_dist = new_dist;
				best_passee = i;
			}
		}
	}
	unsigned int flags = ai_flags::calc_flags(the_world->playtype());
	flags &= ~(ai_flags::clip_play_area);
	if (best_passee == the_team.size()){
		// No robot can receive the pass. Simply chip the ball forward.
		kick::ptr tactic (new kick(the_robots[0]));
		tactic->set_chip();
		tactic->set_target(the_world->field().enemy_goal());
		tactic->set_flags(flags);
		tactic->tick();
	} else {
		pass::ptr tactic (new pass(the_robots[0], the_team[best_passee], the_world));
		tactic->set_flags(flags);
		tactic->tick();
	}
}

void execute_indirect_free_kick_friendly::robots_changed() {
	tick();
}

