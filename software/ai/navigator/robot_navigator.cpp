#include "ai/navigator/robot_navigator.h"
#include "ai/util.h"
#include "geom/util.h"

#include <iostream>
#include <cstdlib>

namespace {

#warning magic constants
	const double AVOID_MULT = 1.0;
	const double AVOID_CONST = 1.0;
	const double ROTATION_THRESH = 100.0 * M_PI / 180.0;
	const double ROTATION_STEP = 1.0 * M_PI / 180.0;
	const double LOOKAHEAD_MAX = robot::MAX_RADIUS * 10;
        const double AVOID_BALL_AMOUNT = 500;
}

robot_navigator::robot_navigator(player::ptr player, world::ptr world) : the_player(player), the_world(world), position_initialized(false), orientation_initialized(false), flags(0) {
}

double robot_navigator::get_avoidance_factor() const {
	return AVOID_CONST + AVOID_MULT * the_player->est_velocity().len();
}

bool robot_navigator::dst_ok(point dst){
  return  (dst - get_inbounds_point(dst)).len() <= ai_util::POS_CLOSE;
}

point robot_navigator::force_defense_len(point dst){
    point temp = dst;
    temp.x = std::max(the_world->field().friendly_goal().x + the_world->field().defense_area_radius(), dst.x);
    return temp;
}

point robot_navigator::force_offense_len(point dst){
    point temp = dst;
    temp.x = std::min(the_world->field().enemy_goal().x - the_world->field().defense_area_radius(), dst.x);
    return temp;
}
point robot_navigator::clip_defense_area(point dst){
 
  if(abs(2*dst.y) <  the_world->field().defense_area_stretch()){
    return force_defense_len(dst);
  }else{
    point a = the_world->field().friendly_goal();
    a.y += the_world->field().defense_area_stretch()/2;
    point dir;
    if( (dst-a).len() <  the_world->field().defense_area_radius() ){
       dir = (dst-a).norm();
       dir*=the_world->field().defense_area_radius();
    }

    a.y -=  the_world->field().defense_area_stretch();

    if( (dst-a).len() <  the_world->field().defense_area_radius() ){
       dir = (dst-a).norm();
       dir*=the_world->field().defense_area_radius();
    }

   if(dir.norm().len()<0.5)return force_defense_len(dst);

   return dir;

  }

}

point robot_navigator::clip_offense_area(point dst){
 
  if(abs(2*dst.y) <  the_world->field().defense_area_stretch()){
    return force_offense_len(dst);
  }else{
    point a = the_world->field().enemy_goal();
    a.y += the_world->field().defense_area_stretch()/2;
    point dir;
    if( (dst-a).len() <  the_world->field().defense_area_radius() ){
       dir = (dst-a).norm();
       dir*=the_world->field().defense_area_radius();
    }

    a.y -=  the_world->field().defense_area_stretch();

    if( (dst-a).len() <  the_world->field().defense_area_radius() ){
       dir = (dst-a).norm();
       dir*=the_world->field().defense_area_radius();
    }

   if(dir.norm().len()<0.5)return force_defense_len(dst);

   return dir;

  }
}

point robot_navigator::get_inbounds_point(point dst){

	const ball::ptr the_ball(the_world->ball());
	const field &the_field(the_world->field());
	const point balldist = the_ball->position() - the_player->position();
	const double distance = (dst - the_player->position()).len();

	point wantdest = dst;

	if (flags & clip_play_area) {
		wantdest = clip_point(target_position, point(-the_field.length()/2 + the_field.bounds_margin(), -the_field.width()/2 + the_field.bounds_margin()),
				point(the_field.length()/2 - the_field.bounds_margin(), the_field.width()/2 - the_field.bounds_margin()));
	}
	if(flags & stay_own_half){
	  wantdest.x =  std::min(wantdest.x, 0.0);
	}
	if(flags & avoid_ball_stop){
	  //need to grab ball distance from somewhere
	  point ball_diff =  the_player->position() -  the_ball->position();
	  //	  point cur_diff = the_ball->position() - 
	  if(ball_diff.len()< AVOID_BALL_AMOUNT){
	    if((the_player->position()-wantdest).dot(ball_diff)>=ai_util::POS_CLOSE){
	      //destination goes away from the ball destination is ok
	      //scale the destination so that we stay far enough away from the ball
	      point from_ball =  (wantdest - the_ball->position());
	      if(from_ball.len() < AVOID_BALL_AMOUNT){
		from_ball =  (wantdest - the_ball->position()).norm()*AVOID_BALL_AMOUNT;
	      }	      
	      wantdest = the_ball->position() + from_ball;
	    }else{
	      //destination goes closer to the ball we don't want that
	      //just move as quickly as possible away from ball
	      wantdest = the_ball->position() + ball_diff.norm()*AVOID_BALL_AMOUNT;
	    }
	  }
	}

	if(flags & avoid_friendly_defence){
	  wantdest = clip_defense_area(wantdest);
	}

	if(flags & avoid_enemy_defence){
	  wantdest = clip_offense_area(wantdest);
	}


	if(flags & penalty_kick_friendly){

	}

	if(flags & penalty_kick_enemy){

	}
	return wantdest;
}

void robot_navigator::tick() {
	const ball::ptr the_ball(the_world->ball());
	const field &the_field(the_world->field());

	const point balldist = the_ball->position() - the_player->position();
	point wantdest = (position_initialized) ? target_position : the_player->position();
	const double wantori = (orientation_initialized) ? target_orientation : atan2(balldist.y, balldist.x);
	wantdest = get_inbounds_point(wantdest);
	const double distance = (wantdest - the_player->position()).len();

	if(ai_util::ball_close(the_world, the_player)){
          #warning magic constant here need to come up with more intelligent dribble strategy
	  the_player->dribble(0.5);
	}else{
	  the_player->dribble(0.0);
	}

	// at least face the ball
	if (distance < ai_util::POS_CLOSE || !position_initialized) {
		if (balldist.len() > ai_util::POS_CLOSE) the_player->move(the_player->position(), wantori);

		// DO NOT FORGET! reset orientation settings.
		orientation_initialized = false;
		position_initialized = false;
		return;
	}

	// reset orientation settings.
	orientation_initialized = false;
	position_initialized = false;

	const point direction = (wantdest - the_player->position()).norm();

	point leftdirection = direction;
	point rightdirection = direction;

	double angle = 0.0;

	bool stop = false;
	bool chooseleft;

	//it shouldn't take that many checks to get a good direction
	while (true) {

		leftdirection = direction.rotate(angle);
		rightdirection = direction.rotate(-angle);

		if (check_vector(the_player->position(), wantdest, leftdirection)) {
			chooseleft = true;
			break;
		} else if (check_vector(the_player->position(), wantdest, rightdirection)) {
			chooseleft = false;
			break;
		}

		if (angle > ROTATION_THRESH) {
			leftdirection = rightdirection = direction;
			stop = true;
			break;
		}
		angle += ROTATION_STEP;
	}

	if(stop) {
		the_player->move(the_player->position(), wantori);
		return;
	}

	const point selected_direction = (chooseleft) ? leftdirection : rightdirection;

	if (angle < ai_util::ORI_CLOSE) {
		the_player->move(wantdest, wantori);
	} else {
		// maximum warp
		the_player->move(the_player->position() + selected_direction * std::min(distance, 1.0), wantori);
	}
}

void robot_navigator::set_position(const point &position) {
	position_initialized = true;
	target_position = position;
}

void robot_navigator::set_orientation(const double &orientation) {
	orientation_initialized = true;
	target_orientation = orientation;
}

// TODO: use the util functions
bool robot_navigator::check_vector(const point& start, const point& dest, const point& direction) const {
	const ball::ptr the_ball(the_world->ball());
	const point startdest = dest - start;
	const double lookahead = std::min(startdest.len(), LOOKAHEAD_MAX);

	if(abs(direction.len() - 1.0) > ai_util::POS_CLOSE) {
		std::cerr << " Direction not normalized! " << direction.len() << std::endl;
		return false;
	}

	const team * const teams[2] = { &the_world->friendly, &the_world->enemy };
	for (unsigned int i = 0; i < 2; ++i) {
		for (unsigned int j = 0; j < teams[i]->size(); ++j) {
			const robot::ptr rob(teams[i]->get_robot(j));
			if(rob == this->the_player) continue;
			const point rp = rob->position() - start;
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);

			if (proj <= 0) continue;

			if (proj < lookahead && perp < get_avoidance_factor() * (robot::MAX_RADIUS * 2)) {
				return false;
			}
		}
	}

	if (flags & avoid_ball_near) {
		const point ballvec = the_ball->position() - start;
		double proj = ballvec.dot(direction);
		if (proj > 0) {
			double perp = sqrt(ballvec.dot(ballvec) - proj * proj);
			if (proj < lookahead && perp < get_avoidance_factor() * (robot::MAX_RADIUS + ball::RADIUS)) {
				//std::cout << "Checked FALSE" << std::endl;
				return false;
			}
		}
	}

	//std::cout << "Checked TRUE" << std::endl;
	return true;
}

