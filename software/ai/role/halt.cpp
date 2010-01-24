/* ==================================================
Update History:

Name               Date               Remark
Kenneth            23 Jan 2010        Initial implementation

===================================================*/

#include "ai/role/halt.h"
#include "ai/tactic/move.h"

halt::halt(ball::ptr ball, field::ptr field, controlled_team::ptr team) : role(ball, field, team) {
}

void halt::tick(){
    for (unsigned int i = 0; i < the_robots.size(); i++)
    {
        /////////////////
        // This cannot stop the robot. Not sure if this is a simulator problem.
        ////////////////////
        tactics[i]->set_position(the_robots[i]->position());
        tactics[i]->tick();
    } 
}

void halt::robots_changed() {
    tactics.clear();
    for (unsigned int i = 0; i < the_robots.size(); i++)
    {
        tactics.push_back(move::ptr (new move(the_ball, the_field, the_team, the_robots[i])));
        tactics[i]->set_position(the_robots[i]->position());
    } 
}

