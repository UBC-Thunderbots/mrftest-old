#include "ai/role/prepare_kickoff_enemy.h"

prepare_kickoff_enemy::prepare_kickoff_enemy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team) {
    starting_positions[0] = point( -1 * the_field->length()/10 , 0);
    starting_positions[1] = point( -1 * the_field->length()/4, -1* the_field->width()/6 );
    starting_positions[2] = point( -1 * the_field->length()/4, the_field->width()/6 );              
    starting_positions[3] = point( -1 * the_field->length() *0.4 , -1 * the_field->width()/4 );
    starting_positions[4] = point( -1 * the_field->length() * 0.4 , the_field->width()/4 );
}

void prepare_kickoff_enemy::tick(){
   for(unsigned int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
   }
}

void prepare_kickoff_enemy::robots_changed() {
    the_tactics.clear();
    for(unsigned int i=0; i<the_robots.size() ; i++) {
        move::ptr tactic( new move(the_ball, the_field, the_team, the_robots[i]));
        the_tactics.push_back(tactic);
        if(i<NUMBER_OF_STARTING_POSITIONS) {
            the_tactics[i]->set_position(starting_positions[i]);
        }
    }
}
