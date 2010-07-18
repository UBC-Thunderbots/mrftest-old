#include "ai/role/pit_stop.h"

PitStop::PitStop(World::ptr world) : the_world(world) {
}

void PitStop::tick(){
   for(unsigned int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
   }
}

void PitStop::robots_changed() {
    the_tactics.clear();
    for(unsigned int i=0; i<robots.size() ; i++) {
        Move::ptr tactic( new Move(robots[i], the_world));
        the_tactics.push_back(tactic);
    }
    
	const Field &the_field(the_world->field());
    double y_pos = -1 * the_field.width()/2 - 0.1;
    for(unsigned int i=0; i<the_tactics.size(); i++) {
        double x_pos = -1 * the_field.length()/2 + 0.2*i;
        the_tactics[i]->set_position( Point(x_pos, y_pos) );
    }
}
