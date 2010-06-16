#include "ai/role/kickoff_friendly.h"
#include <cmath>
#include "ai/tactic/move.h"
#include "ai/flags.h"

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
   if(the_world->playtype() == playtype::prepare_kickoff_friendly ){
     if(!team_compliance()){ 
       std::cout<<"team is not in compliance"<<std::endl;
   //we can set non on our half destinations in order to avoid ball
     //this role itself calculates how to abide by rules which involves 
     //not staying in rules for a period of time
       for(unsigned int i=0; i<the_robots.size(); i++){
	 point dst = the_robots[i]->position();
	 flags |= ai_flags::stay_own_half;
	 if(rule_violation(the_robots[i]->position())){
	   dst = approach_legal_point(the_robots[i]->position(),i);
	   flags &= ~ai_flags::stay_own_half;
	   //flags &= ~ai_flags::avoid_ball_stop;
	 }

       move::ptr move_tactic(new move(the_robots[i], the_world));
       move_tactic->set_position(dst);
       std::cout<<"called move tactic to "<<dst.x << " " <<dst.y<<std::endl;
              std::cout<<"called move tactic to "<<the_robots[i]->position().x << " " <<the_robots[i]->position().y<<std::endl;
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
	   move_tactic->set_position(the_world->ball()->position());
	 }else{
	   move_tactic->set_position(the_robots[i]->position());
	 }
	 move_tactic->set_flags(flags);
	 move_tactic->tick();
       }

	 //don't worry about the robots that comply with the rules for now
     }  
   }else{
     //we are in execute kickoff
     //do something more intelligent here than just nothing
#warning do something more intelligent here execute kickoff than just have one robot chase ball
       unsigned int flags = ai_flags::calc_flags(the_world->playtype());
       for(unsigned int i=0; i<the_robots.size(); i++){
	 move::ptr move_tactic(new move(the_robots[i], the_world));
	 if(i==0){
	   move_tactic->set_position(the_world->ball()->position());
	 }else{
	   move_tactic->set_position(the_robots[i]->position());
	 }
	 move_tactic->set_flags(flags);
	 move_tactic->tick();
       }
	 //don't worry about the robots that comply with the rules for now
     }  
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

  bool compliant = cur_point.x<0.0 && (the_world->ball()->position() - cur_point).len() > circle_radius;
  return !compliant;

}

//enforces that robots go around the centre circle (with ball in the middle) 
//and on their own side of field for a kickoff
point kickoff_friendly::approach_legal_point(point cur_point, unsigned int robot_num){
  const field &the_field(the_world->field());
  point wantdst = cur_point;
  if(cur_point.x>0.0){
    if(fabs(cur_point.y) < 1.2*circle_radius){
      wantdst.y = (cur_point.y < 0) ? -(1.2*circle_radius + AVOID_BUFFER):(1.2*circle_radius + AVOID_BUFFER);
    }else{
      wantdst.x = cur_point.x - 1.0;
    }
  }
  return wantdst;
}
