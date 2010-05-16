#include "ai/role/pit_stop.h"

pit_stop::pit_stop(world::ptr world) : the_world(world) {
}

void pit_stop::tick(){
   for(unsigned int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
   }
}

void pit_stop::robots_changed() {
    the_tactics.clear();
    for(unsigned int i=0; i<the_robots.size() ; i++) {
        move::ptr tactic( new move(the_robots[i], the_world));
        the_tactics.push_back(tactic);
    }
    
	const field &the_field(the_world->field());
    double y_pos = -1 * the_field.width()/2 - 0.1;
    for(unsigned int i=0; i<the_tactics.size(); i++) {
        double x_pos = -1 * the_field.length()/2 + 0.2*i;
        the_tactics[i]->set_position( point(x_pos, y_pos) );
    }
}
