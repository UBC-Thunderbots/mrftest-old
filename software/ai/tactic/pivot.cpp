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

	DoubleParam PIVOT_ORI_CLOSE("Pivot: angle before it goes for ball (degrees)", 20.0, 0.0, 80.0);
	BoolParam PIVOT_USE_NEW("Pivot: use new code", true);
	DoubleParam PIVOT_DIST("Pivot: distance, in robot radius", 1.8, 0.5, 2.0);

	class PivotState : public Player::State {
		public:
			typedef RefPtr<PivotState> Ptr;
			PivotState() : recent_hit_target(false) {
			}
			bool recent_hit_target;
	};
	
}

Point Pivot::calc_pivot_pos(const Point& ballpos, const Point& target) {
	// do all relative to the ball
	const Point tobehind = (ballpos - target).norm(); // points behind
	const double pivotdist = PIVOT_DIST * Robot::MAX_RADIUS + Robot::MAX_RADIUS + Ball::RADIUS;
	return ballpos + tobehind * pivotdist;
}

Pivot::Pivot(Player::Ptr player, World::Ptr world) : Tactic(player), the_world(world), target(world->field().enemy_goal()), avoid_ball_(false) {
}

void Pivot::tick() {
	if (PIVOT_USE_NEW) tick_experimental();
	else tick_old();
}

void Pivot::tick_old() {

	// use robot navigator instead of storing Move Tactic.
	// the reason is that we can't unset flags.
	RobotNavigator navi(player, the_world);

	const Ball::Ptr the_ball(the_world->ball());

	bool recent_hit_target = false;
	PivotState::Ptr state(PivotState::Ptr::cast_dynamic(player->get_state(typeid(*this))));
	if(state)recent_hit_target= state->recent_hit_target;
	else{
		state =PivotState::Ptr(new PivotState());
		player->set_state(typeid(*this), state);
	}

	// if we have the ball then move to the destination
	if (AIUtil::has_ball(the_world, player) && ! avoid_ball_) {
		state->recent_hit_target = true;
		navi.set_position(target);
		navi.set_orientation((target - player->position()).orientation());
		navi.set_flags(flags);
		navi.tick();
		return;
	}

	Point est_ball_pos = the_ball->position();
	Point robot_dst = est_ball_pos;
	Point vec = target - est_ball_pos;

	Point ball_player_diff = (the_ball->position() - player->position());
	Point target_player_diff = (target - player->position());

	const double pivotdist = PIVOT_DIST * Robot::MAX_RADIUS + Robot::MAX_RADIUS + Ball::RADIUS;

	if(vec.len()<0.01){
		//ball already too close to target 
		//don't try and divide by small number
	}else{
		vec = vec/vec.len();
		robot_dst -= vec * pivotdist;
	}

	if((robot_dst-player->position()).len()>0.1 && !player->sense_ball()){
		state->recent_hit_target=false;
	}

	Point player_diff_vector = est_ball_pos- player->position();
	Point target_diff_vector = est_ball_pos- robot_dst;

	// just to prevent my head from blowing up
	Point ball2player = -(est_ball_pos- player->position());
	Point ball2dest = -(est_ball_pos- robot_dst);

	if (angle_diff(ball2player.orientation(), ball2dest.orientation()) < M_PI / 6) {
		//if (player_diff_vector.len() < target_diff_vector.len()) {
		if (player_diff_vector.dot(target_diff_vector) > 0 && !avoid_ball_) {
			state->recent_hit_target = true;
			navi.set_position(the_ball->position());
			// navi.set_orientation((target - player->position()).orientation());
			navi.set_flags(flags);
			navi.tick();
			return;
		}
	}

	if((robot_dst-player->position()).len()<0.01){
		state->recent_hit_target = true;
	}
	// std::cout<<"recent hit: "<<recent_hit_target<<std::endl;

	if (state->recent_hit_target && !avoid_ball_) {
		navi.set_position(the_ball->position());
		navi.set_flags(flags);
	} else {
		navi.set_position(robot_dst);
		navi.set_flags(flags | AIFlags::AVOID_BALL_NEAR);
	} 	

	navi.tick(); 	

}

void Pivot::tick_experimental() {

	// use robot navigator instead of storing Move Tactic.
	// the reason is that we can't unset flags.
	RobotNavigator navi(player, the_world);

	const Point ballpos = the_world->ball()->position();

	/*
	PivotState::Ptr state(PivotState::Ptr::cast_dynamic(player->get_state(typeid(*this))));
	if(!state) {
		state = PivotState::Ptr(new PivotState(false));
		player->set_state(typeid(*this), state);
	}
	*/

	// invalid target
	if ((target - ballpos).len() < AIUtil::POS_EPS) {
		LOG_WARN("ball is already in destination");
		navi.set_flags(flags);
		navi.tick();
		return;
	}

	const Point playerpos = player->position();
	const Point dest = calc_pivot_pos(ballpos, target);

	const Point ball2dest = dest - ballpos;
	Point ball2player = playerpos - ballpos;

	// H A C K
	if (ball2player.len() < AIUtil::POS_CLOSE) {
		const double ori = player->orientation();
		ball2player = -Point(cos(ori), sin(ori));
	}

	// we can do something to the ball now!
	//if (!avoid_ball_ && (dest - playerpos).len() < AIUtil::POS_CLOSE) {
	if (!avoid_ball_ && angle_diff(ball2dest.orientation(), ball2player.orientation()) < degrees2radians(PIVOT_ORI_CLOSE)) {
		if (!AIUtil::has_ball(the_world, player)) {
			navi.set_position(ballpos);
		}
		navi.set_flags(flags);
		navi.tick();
		return;
	}

	navi.set_position(dest);
	navi.set_flags(flags | AIFlags::AVOID_BALL_NEAR);
	navi.tick();
}

