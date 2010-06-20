#include "ai/tactic/pivot.h"
#include "geom/angle.h"
#include "ai/flags.h"
#include "ai/tactic/chase.h"
#include "ai/util.h"
#include "ai/navigator/robot_navigator.h"
#include "util/dprint.h"

#include "uicomponents/param.h"

#include <iostream>
#include <cmath>

namespace {

	const double PIVOT_ORI_CLOSE = 10.0 / 180.0 * M_PI;
	const double PIVOT_ANGLE = M_PI / 2;
	double_param PIVOT_DIST("Pivot Distance", 0.08, 0.1, 0.2);

	class pivot_state : public player::state {
		public:
			typedef Glib::RefPtr<pivot_state> ptr;
			pivot_state(const bool& recent) : recent_hit_target(recent) {
			}
			bool recent_hit_target;
	};

}

pivot::pivot(player::ptr player, world::ptr world) : tactic(player), the_world(world), target(world->field().enemy_goal()), get_ball_(true) {
}

void pivot::tick() {

	// use robot navigator instead of storing move tactic.
	// the reason is that we can't unset flags.
	robot_navigator navi(the_player, the_world);

	const ball::ptr the_ball(the_world->ball());

	bool recent_hit_target = false;
	pivot_state::ptr state(pivot_state::ptr::cast_dynamic(the_player->get_state(typeid(*this))));
	if(state)recent_hit_target= state->recent_hit_target;
	else{
		state =pivot_state::ptr(new pivot_state(false));
		the_player->set_state(typeid(*this), state);
	}

	// if we have the ball then move to the destination
	if (ai_util::has_ball(the_world, the_player)) {
		state->recent_hit_target = true;
		navi.set_position(target);
		navi.set_orientation((target - the_player->position()).orientation());
		navi.set_flags(flags);
		navi.tick();
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

	if (player_diff_vector.len() < target_diff_vector.len()) {
		if (player_diff_vector.dot(target_diff_vector) > 0) {
			state->recent_hit_target = true;
			navi.set_position(the_ball->position());
			navi.set_orientation((target - the_player->position()).orientation());
			navi.set_flags(flags);
			navi.tick();
			return;
		}
	}

	if((robot_dst-the_player->position()).len()<0.01){
		state->recent_hit_target = true;
	}
	// std::cout<<"recent hit: "<<recent_hit_target<<std::endl;

	if (state->recent_hit_target) {
		navi.set_position(the_ball->position());
		navi.set_flags(flags);
	} else {
		navi.set_position(robot_dst);
		navi.set_flags(flags | ai_flags::avoid_ball_near);
	} 	

	navi.tick(); 	

}

void pivot::tick_experimental() {

	// use robot navigator instead of storing move tactic.
	// the reason is that we can't unset flags.
	robot_navigator navi(the_player, the_world);

	const point ballpos = the_world->ball()->position();

	// invalid target
	if ((target - ballpos).len() < ai_util::POS_EPS) {
		LOG_WARN("ball is already in destination");
		navi.set_flags(flags);
		navi.tick();
		return;
	}

	// do all relative to the ball
	const point tobehind = (ballpos - target).norm(); // points behind
	const point toplayer = (the_player->position() - ballpos).norm(); // points to the player
	const double anglediff = angle_diff(tobehind.orientation(), toplayer.orientation()); // angle difference
	const point playertotarget = target - the_player->position();
	const double wantori = (-tobehind).orientation();

	point wantdest = ballpos;

	navi.set_orientation(wantori);

	// we can do something to the ball now!
	if (get_ball_ && ((tobehind.dot(toplayer) > 0 && anglediff < PIVOT_ORI_CLOSE) || ai_util::has_ball(the_world, the_player))) {
		if (!ai_util::has_ball(the_world, the_player)) {
			navi.set_position(ballpos);
		}
		navi.set_flags(flags);
		navi.tick();
		return;
	}

	// now we have to know which direction to take.. left or right
	const double rotangle = std::min(PIVOT_ANGLE, anglediff);
	const double pivotdist = PIVOT_DIST + robot::MAX_RADIUS + ball::RADIUS;
	if (tobehind.cross(toplayer) > 0) {
		// left
		wantdest = ballpos + toplayer.rotate(-rotangle) * pivotdist;
	} else {
		// right
		wantdest = ballpos + toplayer.rotate(rotangle) * pivotdist;
	}

	navi.set_position(wantdest);
	navi.set_flags(flags);
	navi.tick();
}

