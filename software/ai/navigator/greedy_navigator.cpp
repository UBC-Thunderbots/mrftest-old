#include "ai/navigator/greedy_navigator.h"
#include "ai/util.h"
#include "ai/flags.h"
#include "geom/util.h"

#include "uicomponents/param.h"

#include <iostream>
#include <cstdlib>

using namespace AI;
using namespace Navigator;

namespace {

#warning magic constants
	const double ROTATION_THRESH = 100.0 * M_PI / 180.0;
	const double ROTATION_STEP = 1.0 * M_PI / 180.0;

	// as required by the rules
	const double AVOID_BALL_AMOUNT = 0.5 + Robot::MAX_RADIUS;
	DoubleParam AVOID_CONST("navigator: avoid factor const", 1.1, 0.9, 2.0);
	DoubleParam AVOID_MULT("navigator: avoid factor mult", 0.01, 0.0, 10.0);
	DoubleParam LOOKAHEAD_MAX("navigator: max distance to look ahead", Robot::MAX_RADIUS*5, Robot::MAX_RADIUS*1, Robot::MAX_RADIUS*20);

	BoolParam ENEMY_AVOID("navigator: avoid Enemy Near Ball", true);
	BoolParam FRIENDLY_AVOID("navigator: avoid Friendly Near Ball", true);
	DoubleParam NEAR_BALL_THRESHOLD("navigator: distance to be considered near ball", Robot::MAX_RADIUS * 5, Robot::MAX_RADIUS*1, Robot::MAX_RADIUS*20);

	BoolParam CONSTANT_DRIBBLER("dribble: fixed speed", false);
	DoubleParam DRIBBLE_SPEED_LOW("dribble: low speed", 0.25, 0.05, 0.80);
	DoubleParam DRIBBLE_SPEED_RAMP("dribble: ramp speed", 1.00, 0.00, 10.00);
	DoubleParam DRIBBLE_SPEED_MAX("dribble: max speed", 0.60, 0.10, 0.90);

	const double OFFENSIVE_AVOID = 0.2;
	// hardware dependent dribble parameters
	enum {EMPTY = 0, OWN_ROBOT, ENEMY_ROBOT, BALL, ERROR};  
	double correction_distances[5] = {0.0, 1.0, 1.0, 1.0, 0.0};

	double robot_set_point[7] = {0.25, 0.25, 0.25, 0.25, 0.25, 0.25, 0.25};

	DoubleParam BOT_0("BOT0 dribble low", 0.25, 0.05, 0.80);
	DoubleParam BOT_1("BOT1 dribble low", 0.25, 0.05, 0.80);
	DoubleParam BOT_2("BOT2 dribble low", 0.25, 0.05, 0.80);
	DoubleParam BOT_3("BOT3 dribble low", 0.25, 0.05, 0.80);
	DoubleParam BOT_4("BOT4 dribble low", 0.25, 0.05, 0.80);
	DoubleParam BOT_5("BOT5 dribble low", 0.25, 0.05, 0.80);
	DoubleParam BOT_6("BOT5 dribble low", 0.25, 0.05, 0.80);

	inline double get_robot_set_point(int robot_num) {
		if (CONSTANT_DRIBBLER) return DRIBBLE_SPEED_MAX;

		switch(robot_num){
			case 0: return BOT_0;
			case 1: return BOT_1;
			case 2: return BOT_2;
			case 3: return BOT_3;
			case 4: return BOT_4;
			case 5: return BOT_5;
			case 6: return BOT_6;
		};
		/*
		if(robot_num >=0 && robot_num<7){
			return robot_set_point[robot_num];
		}
		*/
		return DRIBBLE_SPEED_LOW;
	}

}



double TeamGreedyNavigator::get_avoidance_factor() const {
	return AVOID_CONST + AVOID_MULT * the_player->est_velocity().len();
}

Point TeamGreedyNavigator::force_defense_len(Point dst){
    Point temp = dst;
    temp.x = std::max(the_world.field().friendly_goal().x + the_world.field().defense_area_radius() + Robot::MAX_RADIUS, dst.x);
    return temp;
}

Point TeamGreedyNavigator::force_offense_len(Point dst){
    Point temp = dst;
    temp.x = std::min(the_world.field().enemy_goal().x - (the_world.field().defense_area_radius()+OFFENSIVE_AVOID), dst.x);
    return temp;
}
Point TeamGreedyNavigator::clip_defense_area(Point dst){

  Point seg[2];
  seg[0] = the_player->position();
  seg[1] = dst;
  Point defense_rect[4];


  for(int i=0; i<4; i++){
    int a = i/2;
    int b = (i%2) ? -1:1;
    defense_rect[i] =  Point(the_world.field().friendly_goal().x + a*(the_world.field().defense_area_radius() +  Robot::MAX_RADIUS),  b*the_world.field().defense_area_stretch()/2.0);
  }

  if(line_seg_intersect_rectangle(seg, defense_rect)){
    return force_defense_len(dst);
  }
	//clip the two quater-circles around the defense area
	Point defense_circA = Point(the_world.field().friendly_goal().x, the_world.field().defense_area_stretch()/2.0);
	Point defense_circB = Point(the_world.field().friendly_goal().x, -(the_world.field().defense_area_stretch()/2.0));
	Point wantdest = clip_circle(defense_circA, the_world.field().defense_area_radius() + Robot::MAX_RADIUS, dst);
	wantdest = clip_circle(defense_circB, the_world.field().defense_area_radius() +  Robot::MAX_RADIUS, wantdest);
	// std::cout << " w" << wantdest << " dA" << defense_circA << " dB" << defense_circB << std::endl;
 
	return wantdest;
	    
}

Point TeamGreedyNavigator::clip_offense_area(Point dst){

  Point seg[2];
  seg[0] = the_player->position();
  seg[1] = dst;
  Point offense_rect[4];

  for(int i=0; i<4; i++){
    int a = i/2;
    int b = (i%2) ? -1:1;
    offense_rect[i] =  Point(the_world.field().enemy_goal().x +OFFENSIVE_AVOID  - a*(the_world.field().defense_area_radius() + OFFENSIVE_AVOID),  b*the_world.field().defense_area_stretch()/2.0);
  }

  if(line_seg_intersect_rectangle(seg, offense_rect)){
    return force_offense_len(dst);
  }

 	//clip the two quater-circles around the defense area
	Point defense_circA = Point(the_world.field().enemy_goal().x, the_world.field().defense_area_stretch()/2.0);
	Point defense_circB = Point(the_world.field().enemy_goal().x, -(the_world.field().defense_area_stretch()/2.0));
	Point wantdest = clip_circle(defense_circA, the_world.field().defense_area_radius() + OFFENSIVE_AVOID, dst);
	wantdest = clip_circle(defense_circB, the_world.field().defense_area_radius() + OFFENSIVE_AVOID, wantdest);
 
  	return wantdest;
  	
}

Point TeamGreedyNavigator::clip_circle(Point circle_centre, double circle_radius, Point dst){

  		Point wantdest = dst;
		Point circle_centre_diff =  the_player->position() -  circle_centre;
		Point ball_dst_diff =  wantdest -  circle_centre;

		if(circle_centre_diff.len()< circle_radius){
			if((the_player->position()-wantdest).dot(circle_centre_diff) < 0){
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
			std::vector<Point> intersections = lineseg_circle_intersect(circle_centre, circle_radius, the_player->position(), wantdest); 
			if(intersections.size()>0){
				wantdest =  intersections[0];
			}	
		}
		return wantdest;

}

Point TeamGreedyNavigator::clip_playing_area(Point wantdest){
	const Field &the_field(the_world.field());
   return  clip_point(wantdest, Point(-the_field.length()/2 + the_field.bounds_margin(), -the_field.width()/2 + the_field.bounds_margin()), Point(the_field.length()/2 - the_field.bounds_margin(), the_field.width()/2 - the_field.bounds_margin()));
}

Point TeamGreedyNavigator::get_inbounds_point(Point dst){

	const Ball::Ptr the_ball(the_world.ball());
	const Field &the_field(the_world.field());
	const Point balldist = the_ball->position() - the_player->position();
	const double distance = (dst - the_player->position()).len();

	Point wantdest = dst;

	if (flags & Flags::CLIP_PLAY_AREA) {
	  wantdest = clip_playing_area(wantdest);
	}

	if (flags & Flags::STAY_OWN_HALF) {
		wantdest.x =  std::min(wantdest.x, -(Robot::MAX_RADIUS + ::AI::Util::POS_CLOSE));
	}

	if (flags & Flags::AVOID_BALL_STOP) {
	  Point before = wantdest;
		wantdest = clip_circle(the_ball->position(), AVOID_BALL_AMOUNT, wantdest);
		//don't go out of bounds in order to comply with the rules
		//just move along in the direction away from the ball while not going out of bounds
		wantdest = clip_playing_area(wantdest);
	}

	if (flags & Flags::AVOID_FRIENDLY_DEFENSE) {
		//std::cout<<"defense before "<<wantdest.x<<" "<<wantdest.y<<std::endl;
		wantdest = clip_defense_area(wantdest);
		//std::cout<<"defense after  "<<wantdest.x<<" "<<wantdest.y<<std::endl;
	}

	if (flags & Flags::AVOID_ENEMY_DEFENSE) {
		wantdest = clip_offense_area(wantdest);
	}

	if (flags & Flags::PENALTY_KICK_FRIENDLY) {
		wantdest.x =  std::min(wantdest.x, the_field.penalty_enemy().x - 0.400 - (Robot::MAX_RADIUS + ::AI::Util::POS_CLOSE));
	}

	if (flags & Flags::PENALTY_KICK_ENEMY) {
		wantdest.x =  std::max(wantdest.x, the_field.penalty_friendly().x + 0.400 + (Robot::MAX_RADIUS + ::AI::Util::POS_CLOSE));
	}

	//make sure that we do not ram into the net posts
	//allow for touching but do not have robots plow through goal net posts	
	wantdest = clip_circle( the_field.friendly_goal_boundary().first, Robot::MAX_RADIUS , wantdest);
	wantdest = clip_circle( the_field.friendly_goal_boundary().second, Robot::MAX_RADIUS , wantdest);
	wantdest = clip_circle( the_field.enemy_goal_boundary().first, Robot::MAX_RADIUS , wantdest);
	wantdest = clip_circle( the_field.enemy_goal_boundary().second, Robot::MAX_RADIUS , wantdest);

	Point dir = wantdest - the_player->position();
	if(dir.x<0.0){
	  if(the_player->position().y >= std::min(the_field.friendly_goal_boundary().first.y, the_field.friendly_goal_boundary().second.y) &&
	     the_player->position().y <=  std::max(the_field.friendly_goal_boundary().first.y, the_field.friendly_goal_boundary().second.y)){
	    wantdest.x = std::max(the_field.friendly_goal_boundary().first.x + Robot::MAX_RADIUS, wantdest.x);
	  }
	}

	return wantdest;
}

void TeamGreedyNavigator::tick(Player::Ptr play) {

  Navigator::Ptr nv = navis[play->address()];
  the_player =play;
  target_position = nv->target_position;
  target_orientation = nv->target_orientation;
  flags = nv->get_flags();
  need_dribble = nv->get_dribbler();

	const Ball::Ptr the_ball(the_world.ball());
	const Field &the_field(the_world.field());

	const Point balldist = the_ball->position() - the_player->position();
	Point wantdest =  target_position.first;
	const double wantori = target_orientation.first;
	wantdest = get_inbounds_point(wantdest);
	const double distance = (wantdest - the_player->position()).len();

	bool wantdribble;
	if (flags & Flags::AVOID_BALL_STOP) {
		wantdribble = false;
	} else {
		wantdribble = need_dribble || ::AI::Util::has_ball(the_world, the_player);
	}

	// dribble when it needs to
#warning has_ball
	if (wantdribble) {
	  const double dribblespeed = std::min<double>(get_robot_set_point(the_player->pattern_index) + DRIBBLE_SPEED_RAMP * the_player->sense_ball_time(), DRIBBLE_SPEED_MAX);
		the_player->dribble(dribblespeed);
	} else if (flags & Flags::AVOID_BALL_STOP) {
		the_player->dribble(0);
	} else {
		the_player->dribble(get_robot_set_point(the_player->pattern_index));
	}



	// at least face the ball
	if (distance < ::AI::Util::POS_EPS) {
		the_player->move(the_player->position(), wantori);
		flags = 0;
		return;
	}

	const Point direction = (wantdest - the_player->position()).norm();

	Point leftdirection = direction;
	Point rightdirection = direction;

	double angle = 0.0;

	bool stop = false;
	bool chooseleft;
        ball_obstacle = false;



	unsigned int right_obstacle = EMPTY;
	unsigned int left_obstacle = EMPTY;
	unsigned int obstacle = EMPTY;

	//it shouldn't take that many checks to get a good direction
	while (true) {

		leftdirection = direction.rotate(angle);
		rightdirection = direction.rotate(-angle);

		ball_obstacle = ball_obstacle ||  check_ball(the_player->position(), wantdest, leftdirection) ;
		ball_obstacle = ball_obstacle ||  check_ball(the_player->position(), wantdest, rightdirection);
		// Don't avoid obstacles if we're close to ball
		ball_obstacle = ball_obstacle || ((the_player->position()-the_ball->position()).len() < NEAR_BALL_THRESHOLD);

		bool left_ok = check_vector(the_player->position(), wantdest, leftdirection);
		bool right_ok = check_vector(the_player->position(), wantdest, rightdirection);

		if (left_ok) {
			chooseleft = true;
			obstacle = left_obstacle;
			break;
		} else if (right_ok) {
			chooseleft = false;
			obstacle = right_obstacle;
			break;
		}

		left_obstacle = check_obstacles(the_player->position(), wantdest, leftdirection);
		right_obstacle = check_obstacles(the_player->position(), wantdest, rightdirection);

		if (angle > ROTATION_THRESH) {
			leftdirection = rightdirection = direction;
			stop = true;
			break;
		}
		angle += ROTATION_STEP;
	}

	if(stop) {
		the_player->move(the_player->position(), wantori);
		//	flags = 0;
		return;
	}

	const Point selected_direction = (chooseleft) ? leftdirection : rightdirection;

	//if (angle < ::AI::Util::ORI_CLOSE) {
	if (angle == 0) {
		the_player->move(wantdest, wantori);
	} else {
	  double correct_amount = correction_distances[obstacle];
		// maximum warp
		the_player->move(the_player->position() + selected_direction * std::min(distance, correct_amount), wantori);
	}

	//	flags = 0;
}

#warning TODO: use the util functions
bool TeamGreedyNavigator::check_vector(const Point& start, const Point& dest, const Point& direction) const {
	return check_obstacles(start, dest, direction) == EMPTY;
}

unsigned int TeamGreedyNavigator::check_obstacles(const Point& start, const Point& dest, const Point& direction) const {
	const Ball::Ptr the_ball(the_world.ball());
	const Point startdest = dest - start;
	const double lookahead = std::min<double>(startdest.len(), LOOKAHEAD_MAX);

	if (abs(direction.len() - 1.0) > ::AI::Util::POS_EPS) {
		std::cerr << " Direction not normalized! " << direction.len() << std::endl;
		return ERROR;
	}

	const Team * const teams[2] = { &the_world.friendly, &the_world.enemy };
	for (unsigned int i = 0; i < 2; ++i) {
	  bool check_avoid = !ball_obstacle;
	  if(i==0){
	    check_avoid = !ball_obstacle ||  FRIENDLY_AVOID;
	  }else{
	    check_avoid = !ball_obstacle ||  ENEMY_AVOID;
	  }

	  if(check_avoid){
		for (unsigned int j = 0; j < teams[i]->size(); ++j) {
			const Robot::Ptr rob(teams[i]->get_robot(j));
			if (rob == the_player) continue;
			const Point rp = rob->position() - start;
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);

			if (proj <= 0) continue;

			if (proj < lookahead && perp < get_avoidance_factor() * (Robot::MAX_RADIUS * 2)) {
				return (i==0) ? OWN_ROBOT:ENEMY_ROBOT;
			}
		}
	  }
	}

	return EMPTY;
}


bool TeamGreedyNavigator::check_ball(const Point& start, const Point& dest, const Point& direction) const {
	const Ball::Ptr the_ball(the_world.ball());
	const Point startdest = dest - start;
	const double lookahead = std::min<double>(startdest.len(), LOOKAHEAD_MAX);

	if (abs(direction.len() - 1.0) > ::AI::Util::POS_EPS) {
		std::cerr << " Direction not normalized! " << direction.len() << std::endl;
		return false;
	}

		const Point ballvec = the_ball->position() - start;
		double proj = ballvec.dot(direction);
		if (proj > 0) {
			double perp = sqrt(ballvec.dot(ballvec) - proj * proj);
			// double distance to ball
			if (proj < lookahead && perp < get_avoidance_factor() * (Robot::MAX_RADIUS + Ball::RADIUS * 2)) {
				return true;
			}
		}
	
	return false;
}
		
  void TeamGreedyNavigator::tick(){
    std::vector<Player::Ptr> pl =  the_world.friendly.get_players();
    for(int i=0; i<pl.size(); i++){
      tick(pl[i]);
    }
  }
