#include "ai/tactic/chase_and_shoot.h"
#include "geom/angle.h"
#include "ai/flags.h"
#include "ai/tactic/chase.h"

#include <iostream>
#include <cmath>

namespace{

	class chase_target_state : public player::state {
	public:
	  typedef Glib::RefPtr<chase_target_state> ptr;
	  chase_target_state(bool recent):recent_hit_target(recent){

	  }
	  bool recent_hit_target;
	};

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
 	

	bool recent_hit_target = false;
	chase_target_state::ptr state(chase_target_state::ptr::cast_dynamic(the_player->get_state(typeid(*this))));
	if(state)recent_hit_target= state->recent_hit_target;
	else{
	  state =chase_target_state::ptr(new chase_target_state(false));
	the_player->set_state(typeid(*this), state);
	}
	
 	
 	//if we have the ball then move to the destination
	if(the_player->sense_ball()){
		move_tactic.set_position(target);
		move_tactic.set_orientation( (target - the_player->position()).orientation() );
		state->recent_hit_target = true;
		return;
 	}
 	

 	point est_ball_pos = the_ball->position();
	point robot_dst = est_ball_pos;
	point vec = target - est_ball_pos;
	
 	point ball_player_diff = (the_ball->position() - the_player->position());
 	point target_player_diff = (target - the_player->position());

 	
 	if(vec.len()<0.01){
 	//ball already too close to target 
 	//don't try and divide by small number
 	}else{
 	vec = vec/vec.len();
 	robot_dst -= vec*0.06;
 	}
 	
 	if((robot_dst-the_player->position()).len()>0.1 && !the_player->sense_ball()){
 	state->recent_hit_target=false;
 	}
 	
 	point player_diff_vector = est_ball_pos- the_player->position();
 	point target_diff_vector = est_ball_pos- robot_dst;
 	
 	if(player_diff_vector.len() < target_diff_vector.len()){
 		if(player_diff_vector.dot(target_diff_vector) > 0){
 			state->recent_hit_target = true;
	 	 	move_tactic.set_position(the_ball->position());
			move_tactic.set_orientation((target - the_player->position()).orientation());
	 		move_tactic.tick();
			return;
 		}
 	}

	if((robot_dst-the_player->position()).len()<0.01){
 		state->recent_hit_target = true;
 	}

	if(state->recent_hit_target){
	  move_tactic.set_position(the_ball->position());
	  move_tactic.set_orientation((target - the_player->position()).orientation());
	  move_tactic.tick();
 	}else{
 		move_tactic.set_position(robot_dst);
		move_tactic.set_flags(ai_flags::avoid_ball_near);
		move_tactic.tick(); 	
 	} 	

}
