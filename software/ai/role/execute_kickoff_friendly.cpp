#include "ai/role/execute_kickoff_friendly.h"
#include "ai/tactic/move.h"
#include "ai/tactic/kick.h"
#include "ai/tactic/chase.h"

namespace {
#warning "The constant should not be hardware dependent"
	const double KICKER_STRENGTH = 0.1;
}

ExecuteKickoffFriendly::ExecuteKickoffFriendly(RefPtr<World> world) : the_world(world) {
	contacted_ball = false;
}

void ExecuteKickoffFriendly::avoid_ball(int index){
	RefPtr<Move> tactic(new Move(robots[index], the_world));
	tactic->set_position(Point( -1 * the_world->field().length()/2, 0));
	the_tactics.push_back(tactic);
}

void ExecuteKickoffFriendly::kick_ball(int index){
	RefPtr<Kick> tactic( new Kick(robots[index], the_world));
	tactic->set_target(Point( the_world->field().length()/10 , 0));
	tactic->set_kick(KICKER_STRENGTH);
	the_tactics.push_back(tactic);
}

void ExecuteKickoffFriendly::chase_ball(int index) {
	RefPtr<Chase> tactic( new Chase(robots[index], the_world));
	the_tactics.push_back(tactic);
}

void ExecuteKickoffFriendly::tick(){
	the_tactics.clear();
	for (size_t i = 0; i < robots.size(); i++){
		// If ball is in play, kicker should not touch the ball again
		// kicker moves back to make way for other players to grab the ball
#warning has ball here
		if (contacted_ball && !robots[i]->sense_ball()) {
			avoid_ball(i);
		} else if (robots[i]->sense_ball()){
			contacted_ball = true;
			kick_ball(i);
		} else{
			chase_ball(i);
		}
	}
	unsigned int flags = AIFlags::calc_flags(the_world->playtype());
	for(unsigned int i=0; i<the_tactics.size(); i++) {
		the_tactics[i]->set_flags(flags);
	        the_tactics[i]->tick();
	}
}

void ExecuteKickoffFriendly::robots_changed() {
	contacted_ball = false;
	tick();
}

