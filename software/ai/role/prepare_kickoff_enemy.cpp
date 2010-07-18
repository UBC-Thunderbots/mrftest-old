#include "ai/role/prepare_kickoff_enemy.h"

PrepareKickoffEnemy::PrepareKickoffEnemy(World::ptr world) : the_world(world) {
	const Field &the_field(the_world->field());
    // The position of the goalie should not be specified.
    // Goalie is in the Goalie Role, not in this Role
    starting_positions[0] = Point( -1 * the_field.length()/4, -1* the_field.width()/6 );
    starting_positions[1] = Point( -1 * the_field.length()/4, the_field.width()/6 );              
    starting_positions[2] = Point( -1 * the_field.length() *0.4 , -1 * the_field.width()/4 );
    starting_positions[3] = Point( -1 * the_field.length() * 0.4 , the_field.width()/4 );
}

void PrepareKickoffEnemy::tick(){
   unsigned int flags = AIFlags::calc_flags(the_world->playtype());
   for(unsigned int i=0; i<the_tactics.size(); i++) {
	the_tactics[i]->set_flags(flags);
        the_tactics[i]->tick();
   }
}

void PrepareKickoffEnemy::robots_changed() {
    the_tactics.clear();
    for(unsigned int i=0; i<the_robots.size() ; i++) {
        Move::ptr tactic( new Move(the_robots[i], the_world));
        the_tactics.push_back(tactic);
        if(i<NUMBER_OF_STARTING_POSITIONS) {
            the_tactics[i]->set_position(starting_positions[i]);
        }
    }
}
