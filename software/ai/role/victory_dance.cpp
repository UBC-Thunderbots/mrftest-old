#include "ai/role/victory_dance.h"

VictoryDance::VictoryDance() {
}

void VictoryDance::tick(){
	for(unsigned int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
    }
}

void VictoryDance::robots_changed() {
    the_tactics.clear();
    for(unsigned int i=0; i<the_robots.size(); i++) {
        Dance::ptr tactic( new Dance(the_robots[i]));
        the_tactics.push_back(tactic);
    }
}
