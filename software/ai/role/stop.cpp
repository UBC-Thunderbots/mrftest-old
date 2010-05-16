/* ==================================================
Update History:

Name               Date               Remark
Kenneth            23 Jan 2010        Initial implementation


===================================================*/

#include "ai/role/stop.h"
#include "ai/tactic/move.h"

stop::stop(world::ptr world) : the_world(world) {
/*    our_goal.x = the_field->length() * -0.45;
    our_goal.y = 0.0;
    centre.x = 0.0;
    centre.y = 0.0;*/
}

void stop::tick(){
	const ball::ptr the_ball(the_world->ball());

    for (unsigned int i = 0; i < the_robots.size(); i++)
    {
        double distance = (points[i]-the_ball->position()).len();
        if (distance < DIST_AWAY_FROM_BALL * 1.1)
        {
            point tempPoint;
            tempPoint = (points[i]-the_ball->position()) / distance * DIST_AWAY_FROM_BALL * 1.2 + the_ball->position();
            fix_if_out_of_bound(tempPoint);
            points[i] = tempPoint;
            tactics[i]->set_position(points[i]);
        }
/*        if ((the_robots[i]->position()-the_ball->position()).len() < DIST_AWAY_FROM_BALL * 1.1)
        {     
            if ((the_ball->position()-centre).len() < DIST_AWAY_FROM_BALL * 1.2)
            {
                tactics[i]->set_position(our_goal);
            }else
            {
                tactics[i]->set_position(centre);
            }    
        }else
        {
            ///////////////////////
            // This cannot stop the robot. Not sure if this is a simulator problem.
            ////////////////////////
            tactics[i]->set_position(the_robots[i]->position());
        }    */
        tactics[i]->tick();
    } 
}

void stop::robots_changed() {
	const ball::ptr the_ball(the_world->ball());

    tactics.clear();
    for (unsigned int i = 0; i < the_robots.size(); i++)
    {
        tactics.push_back(move::ptr (new move(the_robots[i], the_world)));
        // 1.1 is just the allowance.
        double distance = (the_robots[i]->position()-the_ball->position()).len();
        if (distance < DIST_AWAY_FROM_BALL * 1.1)
        {
            point tempPoint;
            tempPoint = (the_robots[i]->position()-the_ball->position()) / distance * DIST_AWAY_FROM_BALL * 1.2 + the_ball->position();
            fix_if_out_of_bound(tempPoint);
            points.push_back(tempPoint);
        }else
        {
            points.push_back(the_robots[i]->position());
        }
        tactics[i]->set_position(points[i]);
        // 1.2 is as an allowance
        // if the ball is close to the centre, move to our goal
        // else move to centre
/*        if ((the_ball->position()-centre).len() < DIST_AWAY_FROM_BALL * 1.2)
        {
            tactics[i]->set_position(our_goal);
        }else
        {
            tactics[i]->set_position(centre);
        }    */
    } 
}

void stop::fix_if_out_of_bound(point& pt)
{
	const field &the_field(the_world->field());
	const ball::ptr the_ball(the_world->ball());

    int count = 0;
    while (pt.x > the_field.length()*0.49 || pt.x < the_field.length() * -0.49 ||
           pt.y > the_field.width()*0.49 || pt.y < the_field.width()* -0.49)
    {
        count++;
        if (count > 4)
        {  //this should be impossible though..
            pt.x = 0.0;
            pt.y = 0.0;
            return;
        }else
        {
            pt = (pt-the_ball->position()).rotate(PI/2.0) + the_ball->position();  //rotate 90 deg if the ball is out of bound.
        }
    }
}
