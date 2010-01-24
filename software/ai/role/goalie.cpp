#include "ai/role/goalie.h"
#include "ai/tactic/move.h"

goalie::goalie(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team), started(false), default_pos(-0.45*the_field->length(),0), centre_of_goal(-0.5*the_field->length(),0){
}

void goalie::tick(){
    /////////////////////////////////
    // Kenneth 23Jan2010
    // This is incomplete, the goalie only stays behind the goal and the ball,
    // it doesn't pass or control the ball yet.
    /////////////////////////////////
    if (the_robots.size()<1)
    {
        return;
    }else
    {
        move::ptr move_tac( new move(the_ball, the_field, the_team, the_robots[0]));
        if (started)
        {
            point tempPoint = the_ball->position()-centre_of_goal;
            tempPoint = tempPoint * (STANDBY_DIST / tempPoint.len());
            tempPoint += centre_of_goal;
            move_tac->set_position(tempPoint);
        }else
        { 
	    move_tac->set_position( point(-0.45*the_field->length(),0));
        }
        curr_tactic = tactic::ptr(move_tac);
        curr_tactic->tick();
    } 
}

void goalie::robots_changed() {
}

void goalie::start_play() {
    started = true;
}
