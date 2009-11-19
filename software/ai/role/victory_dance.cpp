#include "ai/role/victory_dance.h"

victory_dance::victory_dance(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team) {
}

void victory_dance::tick(){
	for(int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
    }
}

void victory_dance::robots_changed() {
    the_tactics.clear();
    for(int i=0; i<the_robots.size(); i++) {
        dance::ptr tactic( new dance(the_ball, the_field, the_team, the_robots[i]));
        the_tactics.push_back(tactic);
    }
}
