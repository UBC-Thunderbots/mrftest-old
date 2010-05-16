#include "ai/role/execute_kickoff_friendly.h"
#include "ai/tactic/move.h"
#include "ai/tactic/kick.h"
#include "ai/tactic/chase.h"

namespace {
#warning "The constant should not be hardware dependent"
	const double KICKER_STRENGTH = 0.1;
}

execute_kickoff_friendly::execute_kickoff_friendly(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team) {
	contacted_ball = false;
}

void execute_kickoff_friendly::avoid_ball(int index){
	move::ptr tactic(new move(the_ball, the_field, the_team, the_robots[index]));
	tactic->set_position(point( -1 * the_field->length()/2, 0));
	the_tactics.push_back(tactic);
}

void execute_kickoff_friendly::kick_ball(int index){
	kick::ptr tactic( new kick(the_ball, the_field, the_team, the_robots[index]));
	tactic->set_target(point( the_field->length()/10 , 0));
	tactic->set_kick(KICKER_STRENGTH);
	the_tactics.push_back(tactic);
}

void execute_kickoff_friendly::chase_ball(int index) {
	chase::ptr tactic( new chase(the_ball, the_field, the_team, the_robots[index]));
	the_tactics.push_back(tactic);
}

void execute_kickoff_friendly::tick(){
	the_tactics.clear();
	for (size_t i = 0; i < the_robots.size(); i++){
		// If ball is in play, kicker should not touch the ball again
		// kicker moves back to make way for other players to grab the ball
		if (contacted_ball && !the_robots[i]->has_ball()) {
			avoid_ball(i);
		} else if (the_robots[i]->has_ball()){
			contacted_ball = true;
			kick_ball(i);
		} else{
			chase_ball(i);
		}
	}
	for(unsigned int i=0; i<the_tactics.size(); i++) {
	        the_tactics[i]->tick();
	}
}

void execute_kickoff_friendly::robots_changed() {
	contacted_ball = false;
	tick();
}

