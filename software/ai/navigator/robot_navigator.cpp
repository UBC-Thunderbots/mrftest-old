#include "ai/navigator/robot_navigator.h"
#include "ai/util.h"
#include "ai/flags.h"
#include "geom/util.h"

#include <iostream>
#include <cstdlib>

namespace {

#warning magic constants
	const double AVOID_MULT = 2.0;
	const double AVOID_CONST = 1.1;
	const double ROTATION_THRESH = 100.0 * M_PI / 180.0;
	const double ROTATION_STEP = 1.0 * M_PI / 180.0;
	const double LOOKAHEAD_MAX = robot::MAX_RADIUS * 10;

// as required by the rules
  const double AVOID_BALL_AMOUNT = 0.5 + robot::MAX_RADIUS;

// hardware dependent dribble parameters
	const double DRIBBLE_SPEED_LOW  = 0.15;
	const double DRIBBLE_SPEED_RAMP = 0.25;
	const double DRIBBLE_SPEED_MAX  = 0.50;


}

robot_navigator::robot_navigator(player::ptr player, world::ptr world) : the_player(player), the_world(world), position_initialized(false), orientation_initialized(false), flags(0) {
}

double robot_navigator::get_avoidance_factor() const {
	return AVOID_CONST + AVOID_MULT * the_player->est_velocity().len();
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

  point seg[2];
  seg[0] = the_player->position();
  seg[1] = dst;
  point defense_rect[4];

  for(int i=0; i<4; i++){
    int a = i/2;
    int b = (i%2) ? -1:1;
    defense_rect[i] =  point(the_world->field().friendly_goal().x + a*the_world->field().defense_area_radius(),  b*the_world->field().defense_area_stretch()/2.0);
  }

  if(line_seg_intersect_rectangle(seg, defense_rect)){
    return force_defense_len(dst);
  }
	//clip the two quater-circles around the defense area
	point defense_circA = point(the_world->field().friendly_goal().x, the_world->field().defense_area_stretch()/2.0);
	point defense_circB = point(the_world->field().friendly_goal().x, -(the_world->field().defense_area_stretch()/2.0));
	point wantdest = clip_circle(defense_circA, the_world->field().defense_area_radius(), dst);
	wantdest = clip_circle(defense_circB, the_world->field().defense_area_radius(), wantdest);
	// std::cout << " w" << wantdest << " dA" << defense_circA << " dB" << defense_circB << std::endl;
 
	return wantdest;
	    
}

point robot_navigator::clip_offense_area(point dst){

  point seg[2];
  seg[0] = the_player->position();
  seg[1] = dst;
  point offense_rect[4];

  for(int i=0; i<4; i++){
    int a = i/2;
    int b = (i%2) ? -1:1;
    offense_rect[i] =  point(the_world->field().enemy_goal().x - a*the_world->field().defense_area_radius(),  b*the_world->field().defense_area_stretch()/2.0);
  }

  if(line_seg_intersect_rectangle(seg, offense_rect)){
    return force_offense_len(dst);
  }

 	//clip the two quater-circles around the defense area
	point defense_circA = point(the_world->field().enemy_goal().x, the_world->field().defense_area_stretch()/2.0);
	point defense_circB = point(the_world->field().enemy_goal().x, -(the_world->field().defense_area_stretch()/2.0));
	point wantdest = clip_circle(defense_circA, the_world->field().defense_area_radius(), dst);
	wantdest = clip_circle(defense_circB, the_world->field().defense_area_radius(), wantdest);
 
  	return wantdest;
  	
}

point robot_navigator::clip_circle(point circle_centre, double circle_radius, point dst){

  		point wantdest = dst;
		point circle_centre_diff =  the_player->position() -  circle_centre;
		point ball_dst_diff =  wantdest -  circle_centre;

		if(circle_centre_diff.len()< circle_radius){
			if((the_player->position()-wantdest).dot(circle_centre_diff) < 0){
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
			std::vector<point> intersections = lineseg_circle_intersect(circle_centre, circle_radius, the_player->position(), wantdest); 
			if(intersections.size()>0){
				wantdest =  intersections[0];
			}	
		}
		return wantdest;

}


point robot_navigator::get_inbounds_point(point dst){

	const ball::ptr the_ball(the_world->ball());
	const field &the_field(the_world->field());
	const point balldist = the_ball->position() - the_player->position();
	const double distance = (dst - the_player->position()).len();

	point wantdest = dst;

	if (flags & ai_flags::clip_play_area) {
		wantdest = clip_point(target_position, point(-the_field.length()/2 + the_field.bounds_margin(), -the_field.width()/2 + the_field.bounds_margin()),
				point(the_field.length()/2 - the_field.bounds_margin(), the_field.width()/2 - the_field.bounds_margin()));
	}
	if (flags & ai_flags::stay_own_half) {
		wantdest.x =  std::min(wantdest.x, 0.0);
	}
	if (flags & ai_flags::avoid_ball_stop) {
		wantdest = clip_circle(the_ball->position(), AVOID_BALL_AMOUNT, wantdest);
	}

	if (flags & ai_flags::avoid_friendly_defence) {
		//std::cout<<"defense before "<<wantdest.x<<" "<<wantdest.y<<std::endl;
		wantdest = clip_defense_area(wantdest);
		//std::cout<<"defense after  "<<wantdest.x<<" "<<wantdest.y<<std::endl;
	}

	if (flags & ai_flags::avoid_enemy_defence) {
		wantdest = clip_offense_area(wantdest);
	}

	if (flags & ai_flags::penalty_kick_friendly) {

	}

	if (flags & ai_flags::penalty_kick_enemy) {

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

	bool wantdribble;
	if (flags & ai_flags::avoid_ball_stop) {
		wantdribble = false;
	} else {
		wantdribble = need_dribble || ai_util::ball_close(the_world, the_player) || ai_util::has_ball(the_player);
	}

	// dribble when it needs to
#warning has_ball
	if (wantdribble) {
		const double dribblespeed = std::min(DRIBBLE_SPEED_LOW + DRIBBLE_SPEED_RAMP * the_player->sense_ball_time(), DRIBBLE_SPEED_MAX);
		the_player->dribble(dribblespeed);
	} else {
		the_player->dribble(DRIBBLE_SPEED_LOW);
	}

	// DO NOT FORGET! reset orientation settings.
	orientation_initialized = false;
	position_initialized = false;
	need_dribble = true;

	// at least face the ball
	if (distance < ai_util::POS_EPS) {
		the_player->move(the_player->position(), wantori);
		flags = 0;
		return;
	}

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
		flags = 0;
		return;
	}

	const point selected_direction = (chooseleft) ? leftdirection : rightdirection;

	if (angle < ai_util::ORI_CLOSE) {
		the_player->move(wantdest, wantori);
	} else {
		// maximum warp
		the_player->move(the_player->position() + selected_direction * std::min(distance, 1.0), wantori);
	}

	flags = 0;
}

// TODO: use the util functions
bool robot_navigator::check_vector(const point& start, const point& dest, const point& direction) const {
	const ball::ptr the_ball(the_world->ball());
	const point startdest = dest - start;
	const double lookahead = std::min(startdest.len(), LOOKAHEAD_MAX);

	if (abs(direction.len() - 1.0) > ai_util::POS_EPS) {
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

	if (flags & ai_flags::avoid_ball_near) {
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

