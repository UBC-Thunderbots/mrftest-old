#include "ai/role/pit_stop.h"

pit_stop::pit_stop(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team) {
}

void pit_stop::tick(){
   for(unsigned int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
   }
}

void pit_stop::robots_changed() {
    the_tactics.clear();
    for(unsigned int i=0; i<the_robots.size() ; i++) {
        move::ptr tactic( new move(the_ball, the_field, the_team, the_robots[i]));
        the_tactics.push_back(tactic);
    }
    
    double y_pos = -1 * the_field->width()/2 - 0.1;
    for(unsigned int i=0; i<the_tactics.size(); i++) {
        double x_pos = -1 * the_field->length()/2 + 0.2*i;
        the_tactics[i]->set_position( point(x_pos, y_pos) );
    }
}
