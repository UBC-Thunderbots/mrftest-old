#include "ai/role/offensive.h"

offensive::offensive(world::ptr world) : the_world(world) {
}

bool offensive::have_ball() {
	const friendly_team &the_team(the_world->friendly);
    for(unsigned int i=0; i<the_team.size(); i++) {
        if(the_team.get_player(i)->has_ball()) {
            return true;
        }
    }
    return false;
}

void offensive::move_towards_goal(int index) {
    move::ptr tactic( new move(the_robots[index], the_world));
    tactic->set_position(point( the_world->field().length()/2, 0));
    the_tactics.push_back(tactic);
}

void offensive::shoot_at_goal(int index) {
    shoot::ptr tactic( new shoot(the_robots[index], the_world));
    the_tactics.push_back(tactic);
}

void offensive::chase_ball(int index) {
    chase_and_shoot::ptr tactic( new chase_and_shoot(the_robots[index], the_world));
    the_tactics.push_back(tactic);
}

double offensive::get_distance_from_goal(int index) {
    point pos = the_robots[index]->position();
    point goal = point(the_world->field().length()/2,0);

    point dist = goal-pos;

    double distance = dist.len();
    return distance;
}

void offensive::tick(){
    the_tactics.clear();
    for(unsigned int i=0; i<the_robots.size(); i++) {
        if(have_ball()) {
            if(the_robots[i]->has_ball()) {
                if(get_distance_from_goal(i)<the_world->field().length()/3) {
                   shoot_at_goal(i);
                }
                else {
                    move_towards_goal(i);
                }
            }
            else {
               move_towards_goal(i); 
            }
        }
        else {
           chase_ball(i);
        }
    }

    for(unsigned int i=0; i<the_tactics.size(); i++) {
        the_tactics[i]->tick();
    }
}

void offensive::robots_changed() {
    tick();
}
