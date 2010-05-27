#include "ai/role/offensive.h"
#include "ai/util.h"

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

void offensive::pass_ball(int index, int receiver){
    pass::ptr tactic (new pass(the_robots[index], the_world, the_robots[receiver]));
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
                    if (ai_util::calc_best_shot(the_robots[i],the_world) < ai_util::SHOOTING_SAMPLE_POINTS){
                        shoot_at_goal(i);
                    }
                    else{
                        unsigned int best_passee = the_robots.size();
                        double best_dist = 1e99;
                        // We will try passing to another offensive robot,
                        // if there is a clear path to the passee and the passee has a clear path to the goal
                        for (unsigned int j = 0; j < the_robots.size(); j++){
                            if (i == j) continue;  
                            if (ai_util::can_pass(the_world,the_robots[j]) 
                                && ai_util::calc_best_shot(the_robots[j],the_world) < ai_util::SHOOTING_SAMPLE_POINTS){
                                double new_dist = (the_robots[j]->position()-the_robots[i]->position()).len()
                                                + get_distance_from_goal(j);
                                if (best_dist > new_dist){
                                    best_dist = new_dist;
                                    best_passee = j;
                                }
                            }
                        }
                        if (best_passee < the_robots.size()){
                            // found suitable passee, make a pass
                            pass_ball(i,best_passee);
                        }
                        else {// no suitable passee 
                             if (get_distance_from_goal(i) < the_world->field().length()/6) {
                                 // very close to goal, so try making a shot anyways
                                 shoot_at_goal(i);
                             } 
                             else { // move closer to the goal
                                 move_towards_goal(i);
                             }
                        }
                    }
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
    unsigned int flags = ai_flags::calc_flags(the_world->playtype());
    if (have_ball())
      flags |= ai_flags::clip_play_area;
    
    for(unsigned int i=0; i<the_tactics.size(); i++) {
	the_tactics[i]->set_flags(flags);
        the_tactics[i]->tick();
    }
}

void offensive::robots_changed() {
    tick();
}
