#include "ai/role/goalie.h"
#include "ai/tactic/move.h"

goalie::goalie(world::ptr world) : the_world(world), started(false), default_pos(-0.45*the_world->field().length(),0), centre_of_goal(-0.5*the_world->field().length(),0){
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
        move move_tac(the_robots[0], the_world);
        if (started)
        {
            point tempPoint = the_world->ball()->position()-centre_of_goal;
            tempPoint = tempPoint * (STANDBY_DIST / tempPoint.len());
            tempPoint += centre_of_goal;
            move_tac.set_position(tempPoint);
        }else
        { 
	    move_tac.set_position( point(-0.45*the_world->field().length(),0));
        }
        move_tac.tick();
    } 
}

void goalie::robots_changed() {
}

void goalie::start_play() {
    started = true;
}
