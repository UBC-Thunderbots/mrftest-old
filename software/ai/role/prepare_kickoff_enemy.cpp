#include "ai/role/prepare_kickoff_enemy.h"

prepare_kickoff_enemy::prepare_kickoff_enemy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team) {
}

void prepare_kickoff_enemy::tick(){
   for(int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
   }
}

void prepare_kickoff_enemy::robots_changed() {
    the_tactics.clear();
    for(int i=0; i<the_robots.size() ; i++) {
        move::ptr tactic( new move(the_ball, the_field, the_team, the_robots[i]));
        the_tactics.push_back(tactic);
    }
    
    the_tactics[0]->set_position(point( -1 * the_field->length()/10 , 0));
    the_tactics[1]->set_position(point( -1 * the_field->length()/4, -1* the_field->width()/6 ));
    the_tactics[2]->set_position(point( -1 * the_field->length()/4, the_field->width()/6 ));
    the_tactics[3]->set_position(point( -1 * the_field->length() *0.4 , -1 * the_field->width()/4 ));
    the_tactics[4]->set_position(point( -1 * the_field->length() * 0.4 , the_field->width()/4 ));
}
