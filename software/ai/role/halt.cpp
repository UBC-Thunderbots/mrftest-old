#include "ai/role/halt.h"
#include "ai/tactic/move.h"

halt::halt(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team) {
}

void halt::tick(){
    for (size_t i = 0; i < the_robots.size(); i++) {
        the_robots[i]->move(the_robots[i]->position(), the_robots[i]->orientation());
    } 
}

void halt::robots_changed() {
}

