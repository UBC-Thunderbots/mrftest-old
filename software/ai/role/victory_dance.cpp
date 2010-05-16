#include "ai/role/victory_dance.h"

victory_dance::victory_dance() {
}

void victory_dance::tick(){
	for(unsigned int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
    }
}

void victory_dance::robots_changed() {
    the_tactics.clear();
    for(unsigned int i=0; i<the_robots.size(); i++) {
        dance::ptr tactic( new dance(the_robots[i]));
        the_tactics.push_back(tactic);
    }
}
