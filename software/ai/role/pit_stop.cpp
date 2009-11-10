#include "ai/role/pit_stop.h"

pit_stop::pit_stop(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team) {
}

void pit_stop::tick(){
   for(int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
   }
}

void pit_stop::set_robots(const std::vector<player::ptr> &robots) {
    role::set_robots(robots);
    
    the_tactics.clear();
    for(int i=0; i<robots.size() ; i++) {
        move::ptr tactic( new move(the_ball, the_field, the_team, robots[i]));
        the_tactics.push_back(tactic);
    }
    
    the_tactics[0]->set_position(point( -1 * the_field->length()/10 , -1 * the_field->width()/2));
    the_tactics[1]->set_position(point( -2 * the_field->length()/10 , -1 * the_field->width()/2));
    the_tactics[2]->set_position(point( -3 * the_field->length()/10 , -1 * the_field->width()/2));
    the_tactics[3]->set_position(point( -4 * the_field->length()/10 , -1 * the_field->width()/2));
    the_tactics[4]->set_position(point( -5 * the_field->length()/10 , -1 * the_field->width()/2));
}
