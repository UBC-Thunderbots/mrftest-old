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

	//const double PIVOT_ORI_CLOSE = 10.0 / 180.0 * M_PI;
	double_param PIVOT_ORI_CLOSE("Pivot: angle before it goes for ball (degrees)", 10.0, 0.0, 20.0);
	bool_param PIVOT_USE_NEW("Pivot: use new code", true);
	double_param PIVOT_DIST("Pivot: distance, in robot radius", 1.5, 0.5, 2.0);

	class pivot_state : public player::state {
		public:
			typedef Glib::RefPtr<pivot_state> ptr;
			pivot_state(const bool& recent) : recent_hit_target(recent) {
			}
			bool recent_hit_target;
	};
	
}

point pivot::calc_pivot_pos(const point& ballpos, const point& target) {
	// do all relative to the ball
	const point tobehind = (ballpos - target).norm(); // points behind
	const double pivotdist = PIVOT_DIST * robot::MAX_RADIUS + robot::MAX_RADIUS + ball::RADIUS;
	return ballpos + tobehind * pivotdist;
}

pivot::pivot(player::ptr player, world::ptr world) : tactic(player), the_world(world), target(world->field().enemy_goal()), avoid_ball_(false) {
}

void pivot::tick() {
	if (PIVOT_USE_NEW) tick_experimental();
	else tick_old();
}

void pivot::tick_old() {

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
	if (ai_util::has_ball(the_world, the_player) && ! avoid_ball_) {
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

	const double pivotdist = PIVOT_DIST * robot::MAX_RADIUS + robot::MAX_RADIUS + ball::RADIUS;

	if(vec.len()<0.01){
		//ball already too close to target 
		//don't try and divide by small number
	}else{
		vec = vec/vec.len();
		robot_dst -= vec * pivotdist;
	}

	if((robot_dst-the_player->position()).len()>0.1 && !the_player->sense_ball()){
		state->recent_hit_target=false;
	}

	point player_diff_vector = est_ball_pos- the_player->position();
	point target_diff_vector = est_ball_pos- robot_dst;

	// just to prevent my head from blowing up
	point ball2player = -(est_ball_pos- the_player->position());
	point ball2dest = -(est_ball_pos- robot_dst);

	if (angle_diff(ball2player.orientation(), ball2dest.orientation()) < M_PI / 6) {
		//if (player_diff_vector.len() < target_diff_vector.len()) {
		if (player_diff_vector.dot(target_diff_vector) > 0 && !avoid_ball_) {
			state->recent_hit_target = true;
			navi.set_position(the_ball->position());
			// navi.set_orientation((target - the_player->position()).orientation());
			navi.set_flags(flags);
			navi.tick();
			return;
		}
	}

	if((robot_dst-the_player->position()).len()<0.01){
		state->recent_hit_target = true;
	}
	// std::cout<<"recent hit: "<<recent_hit_target<<std::endl;

	if (state->recent_hit_target && !avoid_ball_) {
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

	const point playerpos = the_player->position();
	const point dest = calc_pivot_pos(ballpos, target);

	const point ball2dest = dest - ballpos;
	const point ball2player = playerpos - ballpos;

	// we can do something to the ball now!
	//if (!avoid_ball_ && (dest - playerpos).len() < ai_util::POS_CLOSE) {
	if (!avoid_ball_ && angle_diff(ball2dest.orientation(), ball2player.orientation()) < degrees2radians(PIVOT_ORI_CLOSE)) {
		if (!ai_util::has_ball(the_world, the_player)) {
			navi.set_position(ballpos);
		}
		navi.set_flags(flags);
		navi.tick();
		return;
	}

	navi.set_position(dest);
	navi.set_flags(flags | ai_flags::avoid_ball_near);
	navi.tick();
}

