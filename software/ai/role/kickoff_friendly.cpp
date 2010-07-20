#include "ai/role/kickoff_friendly.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/flags.h"
#include "geom/util.h"
#include "ai/util.h"

#include "uicomponents/param.h"

#include <cmath>
#include <iostream>

namespace {

	const double AVOID_BUFFER = 0.1;

	bool player_cmp_function (RefPtr<Player> i,RefPtr<Player> j) { return (i->pattern_index < j->pattern_index); }

}

KickoffFriendly::KickoffFriendly(RefPtr<World> world) : the_world(world){
const Field &the_field(the_world->field());
  circle_radius =  the_field.centre_circle_radius();
}

void KickoffFriendly::tick() {

   unsigned int flags = AIFlags::calc_flags(the_world->playtype());

   std::sort(players.begin(), players.end(),player_cmp_function);
   if(the_world->playtype() == PlayType::PREPARE_KICKOFF_FRIENDLY ||the_world->playtype() == PlayType::PREPARE_KICKOFF_ENEMY){
     if(!team_compliance()){ 
       //we can set non on our half destinations in order to avoid ball
       //this role itself calculates how to abide by rules which involves 
       //not staying in rules for a period of time
       for(unsigned int i=0; i<players.size(); i++){
	 Point dst = players[i]->position();
	 flags |= AIFlags::STAY_OWN_HALF;
	 if(rule_violation(players[i]->position())){
	   dst = approach_legal_point(players[i]->position(),i);
	   flags &= ~AIFlags::STAY_OWN_HALF;
	 }

	 RefPtr<Move> move_tactic(new Move(players[i], the_world));
	 move_tactic->set_position(dst);
	 move_tactic->set_flags(flags);
	 move_tactic->tick();
       }
     }
     else{
#warning do something more intelligent here prepare kickoff than just have one robot chase ball
       unsigned int flags = AIFlags::calc_flags(the_world->playtype());
      
       for(unsigned int i=0; i<players.size(); i++){
	 RefPtr<Move> move_tactic(new Move(players[i], the_world));
	 if(i==0){
	   move_tactic->set_position(clip_circle(players[i]->position(),circle_radius + AVOID_BUFFER,the_world->ball()->position()));
	 }else{
	   Point dst = players[i]->position();
	   dst.x =  -the_world->field().length()/2;
	   move_tactic->set_position(players[i]->position());
	 }
	 move_tactic->set_flags(flags);
	 move_tactic->tick();
       }

	 //don't worry about the robots that comply with the rules for now
     }  
   } else if(the_world->playtype() == PlayType::EXECUTE_KICKOFF_FRIENDLY) {
	   //we are in execute kickoff
	   unsigned int flags = AIFlags::calc_flags(the_world->playtype());
	   // Set lowest numbered robot without chicker fault to be kicker
	   if (players[0]->chicker_ready_time() >= Player::CHICKER_FOREVER){
		   for (size_t i = 1; i < players.size(); ++i)
			   if (players[i]->chicker_ready_time() < Player::CHICKER_FOREVER){
				   swap(players[0],players[i]);
				   break;
			   }
	   }

	   // NO FLAGS
	   // handle kicker separately
	   // kicker will just force shoot the ball
	   RefPtr<Shoot> shoot_tactic(new Shoot(players[0], the_world));
	   // shoot_tactic->set_flags(flags & ~AIFlags::STAY_OWN_HALF & ~AIFlags::AVOID_BALL_STOP);
	   if (the_world->playtype_time() > AIUtil::PLAYTYPE_WAIT_TIME) {
		   shoot_tactic->force();
	   }
	   shoot_tactic->tick();

	   for(unsigned int i=1; i<players.size(); i++){
		   RefPtr<Move> move_tactic(new Move(players[i], the_world));
		   move_tactic->set_position(players[i]->position());
		   move_tactic->set_flags(flags);
		   move_tactic->tick();
	   }
	   //don't worry about the robots that comply with the rules for now
   } // execute kickoff enemy isn't here; we use normal play assignment instead
}

bool KickoffFriendly::team_compliance(){
  for(int i=0; i< players.size(); i++){
    if(rule_violation(players[i]->position())){
      return false;
    }
  }
  return true;
}

bool KickoffFriendly::rule_violation(Point cur_point){

  bool compliant = cur_point.x < -(Robot::MAX_RADIUS+AIUtil::POS_CLOSE) && (the_world->ball()->position() - cur_point).len() > circle_radius;
  return !compliant;

}

//enforces that robots go around the centre circle (with ball in the middle) 
//and on their own side of field for a kickoff
Point KickoffFriendly::approach_legal_point(Point cur_point, unsigned int robot_num){
	const Field &the_field(the_world->field());
	Point wantdst = cur_point;
	if(cur_point.x>-Robot::MAX_RADIUS) {
		wantdst.y = (cur_point.y < 0) ? -(2.0*circle_radius):(2.0*circle_radius);
		//line the robots up at different widths on the field	
		if(wantdst.y<0.0){
		  wantdst.y -= (the_field.width()/2 - fabs(wantdst.y))/5.0;
		}else{		
		  wantdst.y += (the_field.width()/2 - fabs(wantdst.y))/5.0;
		}

		//go to the halfway point on the field
		if(fabs(cur_point.y) > circle_radius) {
			// go to the left
		  wantdst.x = -the_field.length()/2; 
		}

	} else {
		// put the target position here!!
		 wantdst = clip_circle(cur_point, circle_radius + AVOID_BUFFER, wantdst);
		 wantdst.x = -the_field.length()/2; 
	}
	return wantdst;

}



Point KickoffFriendly::clip_circle(Point cur_point, double circle_radius, Point dst){
  Point circle_centre;
  circle_centre.x = 0.0;
  circle_centre.y = 0.0;

  		Point wantdest = dst;
		Point circle_centre_diff =  cur_point -  circle_centre;
		Point ball_dst_diff =  wantdest -  circle_centre;

		if(circle_centre_diff.len()< circle_radius){
			if((cur_point-wantdest).dot(circle_centre_diff) < 0){
				//destination goes away from the ball destination is ok
				//scale the destination so that we stay far enough away from the ball
				Point from_centre =  (wantdest - circle_centre);

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
			std::vector<Point> intersections = lineseg_circle_intersect(circle_centre, circle_radius, cur_point, wantdest); 
			if(intersections.size()>0){
				wantdest =  intersections[0];
			}	
		}
		return wantdest;

}
