#include "ai/tactic/chase_and_shoot.h"
#include "geom/angle.h"

#include <iostream>

chase_and_shoot::chase_and_shoot(player::ptr player, world::ptr world) : the_world(world), the_player(player), move_tactic(player, world) {


const field &the_field(the_world->field());
target.x=(the_field.length()/2.0);
target.y=( 0.0);

}

bool recent_hit_target = false;

void chase_and_shoot::tick()
{
	const ball::ptr the_ball(the_world->ball());
//	std::cout << (the_ball->position()+the_ball->est_velocity()/2) << std::endl;	
//	std::cout << the_ball->est_velocity() << std::endl;
	//move_tactic->set_position(the_ball->position()+the_ball->est_velocity()/2);
	// predict ball position based on velocity and accleration
 	//NOTE THAT MOVING BALL MANUALLY WITH CURSOR CAN GIVE VERY LARGE VELOCITY AND ACCELERATION
 	

// 	shoot::ptr tactic( new shoot(the_ball, the_field, the_team, the_player));
 //	//std::cout <<"player has ball"<<std::endl;
 //	tactic->tick();
 	

 	
 	
 	 	point est_ball_pos = the_ball->position()+the_ball->est_velocity()/2 + the_ball->est_acceleration()/8;
 	point robot_dst = est_ball_pos;
 	point vec = target - est_ball_pos;
 	
 	if(vec.len()<0.01){
 	//ball already too close to net 
 	//don't try and divide by small number
 	}else{
 	vec = vec/vec.len();
 	robot_dst -= vec*0.16;
 	}
 	
 	if((robot_dst-the_player->position()).len()>0.2){
 	recent_hit_target=false;
 	}
 	
 	if((robot_dst-the_player->position()).len()<0.01){
 		recent_hit_target = true;
 		//std::cout<<"right there!!!"<<std::endl;
 	 	shoot tactic(the_player, the_world);
 		//std::cout <<"player has ball"<<std::endl;
 		tactic.tick();
 	}else if(recent_hit_target ){
 		//std::cout<<"right there!!!"<<std::endl;
 	 	shoot tactic(the_player, the_world);
 		//std::cout <<"player has ball"<<std::endl;
 		tactic.tick();
 	}else{
 		move_tactic.set_position(robot_dst);
		move_tactic.set_avoid_ball(true);
		move_tactic.tick(); 	
 	
 	}
 	

 	
 	
 	
 	

}
