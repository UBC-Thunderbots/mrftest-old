#include "ai/role/kickoff_friendly.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/flags.h"
#include "geom/util.h"
#include "ai/util.h"

#include <cmath>
#include <iostream>

namespace{

  const double AVOID_BUFFER = 0.1;
  bool player_cmp_function (player::ptr i,player::ptr j) { return (i->pattern_index < j->pattern_index); }

}

kickoff_friendly::kickoff_friendly(world::ptr world) : the_world(world){
const field &the_field(the_world->field());
  circle_radius =  the_field.centre_circle_radius();
}

void kickoff_friendly::tick(){


   unsigned int flags = ai_flags::calc_flags(the_world->playtype());

   std::sort(the_robots.begin(), the_robots.end(),player_cmp_function);
   if(the_world->playtype() == playtype::prepare_kickoff_friendly ||the_world->playtype() == playtype::prepare_kickoff_enemy){
     if(!team_compliance()){ 
       //we can set non on our half destinations in order to avoid ball
       //this role itself calculates how to abide by rules which involves 
       //not staying in rules for a period of time
       for(unsigned int i=0; i<the_robots.size(); i++){
	 point dst = the_robots[i]->position();
	 flags |= ai_flags::stay_own_half;
	 if(rule_violation(the_robots[i]->position())){
	   dst = approach_legal_point(the_robots[i]->position(),i);
	   flags &= ~ai_flags::stay_own_half;
	 }

	 move::ptr move_tactic(new move(the_robots[i], the_world));
	 move_tactic->set_position(dst);
	 move_tactic->set_flags(flags);
	 move_tactic->tick();
       }
     }
     else{
#warning do something more intelligent here prepare kickoff than just have one robot chase ball
       unsigned int flags = ai_flags::calc_flags(the_world->playtype());
      
       for(unsigned int i=0; i<the_robots.size(); i++){
	 move::ptr move_tactic(new move(the_robots[i], the_world));
	 if(i==0){
	   move_tactic->set_position(clip_circle(the_robots[i]->position(),circle_radius + AVOID_BUFFER,the_world->ball()->position()));
	 }else{
	   move_tactic->set_position(the_robots[i]->position());
	 }
	 move_tactic->set_flags(flags);
	 move_tactic->tick();
       }

	 //don't worry about the robots that comply with the rules for now
     }  
   }else if(the_world->playtype() == playtype::execute_kickoff_friendly){
     //we are in execute kickoff
       unsigned int flags = ai_flags::calc_flags(the_world->playtype());

       // handle kicker separately
       // kicker will just force shoot the ball
       shoot::ptr shoot_tactic(new shoot(the_robots[0], the_world));
       shoot_tactic->set_flags(flags & ~ai_flags::stay_own_half);
       shoot_tactic->force();
       shoot_tactic->tick();
       for(unsigned int i=1; i<the_robots.size(); i++){
	 move::ptr move_tactic(new move(the_robots[i], the_world));
	 move_tactic->set_position(the_robots[i]->position());
	 move_tactic->set_flags(flags);
	 move_tactic->tick();
       }
	 //don't worry about the robots that comply with the rules for now
   } // execute kickoff enemy isn't here; we use normal play assignment instead
}


bool kickoff_friendly::team_compliance(){
  for(int i=0; i< the_robots.size(); i++){
    if(rule_violation(the_robots[i]->position())){
      return false;
    }
  }
  return true;
}

bool kickoff_friendly::rule_violation(point cur_point){

  bool compliant = cur_point.x < -(robot::MAX_RADIUS+ai_util::POS_CLOSE) && (the_world->ball()->position() - cur_point).len() > circle_radius;
  return !compliant;

}

//enforces that robots go around the centre circle (with ball in the middle) 
//and on their own side of field for a kickoff
point kickoff_friendly::approach_legal_point(point cur_point, unsigned int robot_num){
	const field &the_field(the_world->field());
	point wantdst = cur_point;
	if(cur_point.x>-robot::MAX_RADIUS) {
		wantdst.y = (cur_point.y < 0) ? -(2.0*circle_radius):(2.0*circle_radius);
		if(fabs(cur_point.y) > circle_radius) {
			// go to the left
			wantdst.x = cur_point.x - 1.0;
		}
	} else {
		// put the target position here!!
		 wantdst = clip_circle(cur_point, circle_radius + AVOID_BUFFER, wantdst);
	}
	return wantdst;

}



point kickoff_friendly::clip_circle(point cur_point, double circle_radius, point dst){
  point circle_centre;
  circle_centre.x = 0.0;
  circle_centre.y = 0.0;

  		point wantdest = dst;
		point circle_centre_diff =  cur_point -  circle_centre;
		point ball_dst_diff =  wantdest -  circle_centre;

		if(circle_centre_diff.len()< circle_radius){
			if((cur_point-wantdest).dot(circle_centre_diff) < 0){
				//destination goes away from the ball destination is ok
				//scale the destination so that we stay far enough away from the ball
				point from_centre =  (wantdest - circle_centre);

				//try and head towards the destination
				//scaling the destination if necessary
				if(from_centre.len() < circle_radius){
					from_centre =  (wantdest - circle_centre).norm()*circle_radius;
				}	

				wantdest = circle_centre + from_centre;
			}else{
				//destination goes closer to the ball we don't want that
				//just move as quickly as possible away from ball
				wantdest = circle_centre + circle_centre_diff.norm()*circle_radius;
			}
		}else {
			std::vector<point> intersections = lineseg_circle_intersect(circle_centre, circle_radius, cur_point, wantdest); 
			if(intersections.size()>0){
				wantdest =  intersections[0];
			}	
		}
		return wantdest;

}
