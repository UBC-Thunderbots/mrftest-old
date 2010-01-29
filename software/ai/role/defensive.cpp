#include "ai/role/defensive.h"
#include "ai/role/goalie.h"

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
    // Kenneth: PLEASE READ THIS BEFORE UPDATING.
    // ALSO READ DEFENSIVE.H FOR MORE DETAILS.
    // 1) the_goalie is guaranteed to be non-empty
    // 2) the_robots may be empty(i.e. the defensive role only has one goalie, which is stored separately). In this case the defensive role should just tick the goalie role and do nothing else.
    // 3) If the goalie has ball and there are at least two robots on the field, the the_robots has at least one robot.
    
    if (the_goalie->has_ball())
    {
       if (the_robots.size()==0) // there is no one to pass to
       {
          //TODO the goalie is the only robot in the field, it should probably kick the ball to the other side of the field ASAP...but this is up to you. 
       }else
       {
       //TODO decide whether you want the goalie to pass, or do something else...
       //You can set it to use any tactic and tick it. (you can do that anywhere if you like as long as you don't set it to a goalie role)
       }

       // if you still want the goalie to to use the goalie role, just copy the following code.
    }
    else
    {
       // if the goalie doesn't have ball, it should act like a goalie.
       
       goalie::ptr temp_role = goalie::ptr(new goalie(the_ball, the_field, the_team));
       temp_role->start_play(); 
       goalie_role = role::ptr(temp_role);
       std::vector<player::ptr> goalie_only;
       goalie_only.push_back(the_goalie);
       goalie_role->set_robots(goalie_only);
       goalie_role->tick();
    }

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

void defensive::set_goalie(const player::ptr goalie)
{
    the_goalie = goalie;
}
