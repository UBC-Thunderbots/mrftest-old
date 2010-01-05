#include "ai/role/defensive.h"

defensive::defensive(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team) {
}

bool defensive::have_ball() {
    for(unsigned int i=0; i<the_team->size(); i++) {
        if(the_team->get_player(i)->has_ball()) {
            return true;
        }
    }
    return false;
}

void defensive::move_halfway_between_ball_and_our_goal(int index) {
    move::ptr tactic( new move(the_ball, the_field, the_team, the_robots[index]));

    double x_pos  = -1*the_field->length()/2 + (the_field->length()/2 + the_ball->position().x) /2;
    
    double y_pos = the_robots[index]->position().y;

    tactic->set_position(point(x_pos, y_pos));
    the_tactics.push_back(tactic);
}

void defensive::chase_ball(int index) {
    chase::ptr tactic( new chase(the_ball, the_field, the_team, the_robots[index]));
    the_tactics.push_back(tactic);
}

void defensive::block(int index) {
    
}

double defensive::get_distance_from_ball(int index) {
    point pos = the_robots[index]->position();
    point ball = the_ball->position();

    point dist = ball-pos;

    double distance = dist.len();
    return distance;
}

void defensive::tick(){
    the_tactics.clear();
    for(unsigned int i=0; i<the_robots.size(); i++) {
        if(have_ball()) {
            if(the_robots[i]->has_ball()) {
                //todo: pass
            }
            else {
               move_halfway_between_ball_and_our_goal(i);
            }
        }
        else {
            if(get_distance_from_ball(i) < the_field->length()/4)
            {
                chase_ball(i);
            }
            else
            {
                //todo: block instead
                move_halfway_between_ball_and_our_goal(i);
            }
        }
    }

    for(unsigned int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
    }
}

void defensive::robots_changed() {
    tick();
}
