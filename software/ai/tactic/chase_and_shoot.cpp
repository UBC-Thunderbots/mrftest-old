#include "ai/tactic/chase_and_shoot.h"
#include "geom/angle.h"
#include "ai/flags.h"

#include <iostream>
#include <cmath>

namespace{

	const double SLOW_BALL = 0.07;
	const double LINED_UP_TOL = 2.0*M_PI/180; 

	bool just_chase(ball::ptr the_ball, player::ptr the_player, point target){
 		point ball_player_diff = (the_ball->position() - the_player->position());
 		point target_player_diff = (target - the_player->position());
		bool ball_lined_up = true;	
 		//is the ball the same direction (within LINED_UP_TOL) from the robot as the target
 		bool ball_target_lined_up = angle_diff( ball_player_diff.orientation() , target_player_diff.orientation() ) < LINED_UP_TOL;
 		ball_lined_up &= ball_target_lined_up;
 		bool ball_slow = (the_ball->est_velocity()).len() < SLOW_BALL;
 		bool ball_vel_inline = angle_diff( the_ball->est_velocity().orientation() , (target-the_ball->position()).orientation() ) < LINED_UP_TOL ||
 					angle_diff( (-(the_ball->est_velocity())).orientation() , (target-the_ball->position()).orientation() ) < LINED_UP_TOL;
 		ball_lined_up &= ball_slow || ball_vel_inline;
		return ball_lined_up;
	}
}

chase_and_shoot::chase_and_shoot(player::ptr player, world::ptr world) : tactic(player), the_world(world), move_tactic(player, world) {


const field &the_field(the_world->field());
target.x=(the_field.length()/2.0);
target.y=( 0.0);

}

bool recent_hit_target = false;

void chase_and_shoot::tick()
{
	const ball::ptr the_ball(the_world->ball());
 	
 	
 	//if we have the ball then move to the destination
	if(the_player->sense_ball()){
		move_tactic.set_position(target);
		move_tactic.set_orientation( (target - the_player->position()).orientation() );
		return;
 	}
 	

 	point est_ball_pos = the_ball->position();
	point robot_dst = est_ball_pos;
	point vec = target - est_ball_pos;
	
 	point ball_player_diff = (the_ball->position() - the_player->position());
 	point target_player_diff = (target - the_player->position());
 	
 	
 	 //if the ball lines up perfectly with where our target is
 	// and it's velocity does not throw it off then great move towards the ball
 	
	if(just_chase(the_ball, the_player, target)){
		move_tactic.set_position(the_ball->position());
		move_tactic.set_orientation( (target - the_player->position()).orientation());
		return;
	}
 	
 	


 	
 	if(vec.len()<0.01){
 	//ball already too close to target 
 	//don't try and divide by small number
 	}else{
 	vec = vec/vec.len();
 	robot_dst -= vec*0.06;
 	}
 	
 	if((robot_dst-the_player->position()).len()>0.2){
 	recent_hit_target=false;
 	}
 	
 	point player_diff_vector = est_ball_pos- the_player->position();
 	point target_diff_vector = est_ball_pos- robot_dst;
 	
 	if(player_diff_vector.len() < target_diff_vector.len()){
 		if(player_diff_vector.dot(target_diff_vector) > 0){
 			recent_hit_target = true;
	 	 	 shoot tactic(the_player, the_world);
	 		tactic.tick();
	 		return;
 		}
 	}
 	
 	if((robot_dst-the_player->position()).len()<0.01){
 		recent_hit_target = true;
 		//std::cout<<"right there!!!"<<std::endl;
 	 	//shoot tactic(the_player, the_world);
 		//std::cout <<"player has ball"<<std::endl;
 		//tactic.tick();
 	}else if(recent_hit_target ){
 		//std::cout<<"right there!!!"<<std::endl;
 	 	//shoot tactic(the_player, the_world);
 		//std::cout <<"player has ball"<<std::endl;
 		//tactic.tick();
 	}else{
 		move_tactic.set_position(robot_dst);
		move_tactic.set_flags(ai_flags::avoid_ball_near);
		move_tactic.tick(); 	
 	
 	}
 	

 	
 	
 	
 	

}
