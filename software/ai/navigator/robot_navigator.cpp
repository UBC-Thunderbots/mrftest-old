#include "ai/navigator/robot_navigator.h"
#include <iostream>
#include <cstdlib>

namespace {
	const double EPS = 1e-5;
	const double NEAR = 1e-2;

#warning "hardware dependent parameter"
	const double SLOW_AVOIDANCE_SPEED=1.0;
	const double FAST_AVOIDANCE_SPEED=2.0;
}

robot_navigator::robot_navigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team) : navigator(player, field, ball, team),
	dest_initialized(false), outofbounds_margin(field->width() / 20.0),
	avoidance_factor(2), slow_avoidance_factor(0.5), fast_avoidance_factor(2.0) {

	rotation_angle = 1.0 * PI / 180.0;
	rotation_thresh = 100.0 * PI / 180.0;

	lookahead_step = robot::MAX_RADIUS * 0.5;
	lookahead_max = robot::MAX_RADIUS * 10;
}

double robot_navigator::get_avoidance_factor() const {
	return 1.5;

#warning "delete the code below"

	double robot_speed = the_player->est_velocity().len();

	if(robot_speed < SLOW_AVOIDANCE_SPEED){
		return get_slow_avoidance_factor();
	}

	if(robot_speed > FAST_AVOIDANCE_SPEED){
		return get_fast_avoidance_factor();
	}

	assert(SLOW_AVOIDANCE_SPEED < FAST_AVOIDANCE_SPEED);

	double slowness = (robot_speed - FAST_AVOIDANCE_SPEED)/(SLOW_AVOIDANCE_SPEED - FAST_AVOIDANCE_SPEED);
	double fastness = ( SLOW_AVOIDANCE_SPEED - robot_speed)/(SLOW_AVOIDANCE_SPEED - FAST_AVOIDANCE_SPEED);

	return get_slow_avoidance_factor()*slowness + get_fast_avoidance_factor()*fastness;
}

void robot_navigator::tick() {

	if(!dest_initialized) return;

	point nowdest;

	point balldist = the_ball->position() - the_player->position();

	// if we have the ball, adjust our destination to ensure that we
	// don't take the ball out of bounds, otherwise, head to our
	// assigned destination
	if (the_player->has_ball()) {
		nowdest = clip_point(curr_dest, point(-the_field->length()/2 + outofbounds_margin, -the_field->width()/2 + outofbounds_margin),
				point(the_field->length()/2 - outofbounds_margin, the_field->width()/2 - outofbounds_margin));
	} else {
		nowdest = curr_dest;
	}

	point direction = nowdest - the_player->position();

	// at least face the ball
	if (direction.len() < NEAR) {
		if (balldist.len() > NEAR) the_player->move(the_player->position(), atan2(balldist.y, balldist.x));
		return;
	}

	double dirlen = direction.len();
	direction = direction / direction.len();

	point leftdirection = direction;
	point rightdirection = direction;

	double angle = 0.0;

	bool undiverted = true;
	bool stop = false;
	bool chooseleft;

	while (true) {
		//std::cout << "path changed" <<std::endl;

		//it shouldn't take that many checks to get a good direction
		leftdirection = direction.rotate(angle);
		rightdirection = direction.rotate(-angle);

		if (check_vector(the_player->position(), nowdest, leftdirection)) {
			chooseleft = true;
			break;
		} else if (check_vector(the_player->position(), nowdest, rightdirection)) {
			chooseleft = false;
			break;
		}

		// if we can't find a path within 90 degrees
		// go straight towards our destination
		if (angle > rotation_thresh) {
			leftdirection = rightdirection = direction;
			stop = true;
			break;
		}
		angle += rotation_angle;
	}

	undiverted = angle < EPS;

	if(stop) {
		the_player->move(the_player->position(), atan2(balldist.y, balldist.x));
		return;
	}

	point selected_direction = (chooseleft) ? leftdirection : rightdirection;

	if (undiverted) {
		the_player->move(nowdest, atan2(balldist.y, balldist.x));
	} else {
		// maximum warp
		the_player->move(the_player->position() + selected_direction * std::min(dirlen,1.0), atan2(balldist.y, balldist.x));
	}
}

void robot_navigator::set_point(const point &destination) {
	//set new destinatin point
	dest_initialized = true;
	/*curr_dest = clip_point(destination,
	  point(-the_field->length()/2,-the_field->width()/2),
	  point(the_field->length()/2,the_field->width()/2));*/
	curr_dest = destination;
}

/**
  intended to specify how far robot should travel after making a path correction
  not final! 
  implement or delete function soon

  \param correction_size - amount of distance travelled to correct path

 **/
void robot_navigator::set_correction_step_size(double) {
#warning "implement or delete function soon"
}
/**
  normally the navigator sets the robot orientation to be towards the ball
  use this if you want to override this behaviour
  this only sets the desired orientation for one timestep
  \param orientation
 */
void robot_navigator::set_desired_robot_orientation(double) {
#warning "implement function"
}

bool robot_navigator::robot_avoids_ball() {
	return avoid_ball;
}

void robot_navigator::set_robot_avoids_ball(bool avoid){
	avoid_ball=avoid;
}

/**
  get whether the robot avoid it's own goal
 */
bool robot_navigator::robot_avoids_goal() {
#warning "implement or delete function soon"
	return false;
}

/**
  make the robot avoid it's own goal
  \param avoid whether to avoid it's own goal
 */
void robot_navigator::set_robot_avoids_goal(bool) {
#warning "implement or delete function soon"
}

/**
  get whether the robot is set to stay on it's own half
 */
bool robot_navigator::robot_stays_on_own_half() {
#warning "implement function"
	return false;
}

/**
  make the robot stay on it's own half
  \param avoid whether to make robot stay on it's own half
 */
void robot_navigator::set_robot_stays_on_own_half(bool) {
#warning "implement function"
}

/**
  get whether the robot avoid it's opponents goal
 */
bool robot_navigator::robot_stays_away_from_opponent_goal() {
#warning "implement function"
	return false;
}

/**
  make the robot avoid it's opponents goal
  \param avoid whether to avoid it's opponents goal
 */
void robot_navigator::set_robot_stays_away_from_opponent_goal(bool) {
#warning "implement function"
}

/**
  set how much the robot should avoid the opponents goal by
  \param amount the amount that the robot should avoid goal by
 */
void robot_navigator::set_robot_avoid_opponent_goal_amount(double) {
#warning "implement function"
}

point robot_navigator::clip_point(const point& p, const point& bound1, const point& bound2) {

	double minx = std::min(bound1.x, bound2.x);
	double miny = std::min(bound1.y, bound2.y);
	double maxx = std::max(bound1.x, bound2.x);
	double maxy = std::max(bound1.y, bound2.y);

	point ret = p;

	if (p.x < minx) ret.x = minx;
	else if (p.x > maxx) ret.x = maxx;      

	if (p.y < miny) ret.y = miny;
	else if (p.y > maxy) ret.y = maxy;

	return ret;
}

/*
Collision avoidance seems wrong.
*/
bool robot_navigator::check_vector(const point& start, const point& dest, const point& direction) const {
#warning "fixme"
	const point startdest = dest - start;
	const double lookahead = std::min(startdest.len(), lookahead_max);

	assert(abs(direction.len() - 1.0) < EPS);

	for (int s = 0; s * lookahead_step < lookahead; ++s) {
		for (size_t i = 0; i < the_team->size() + the_team->other()->size(); i++) {
			robot::ptr rob;
			if (i >= the_team->size()) {
				rob = the_team->other()->get_robot(i - the_team->size());
			} else {
				rob = the_team->get_robot(i);
			}
			if(rob == this->the_player) continue;
			const point rp = rob->position() - start;
			const double len = rp.dot(direction);

			if (len <= 0) continue;
			const double d = sqrt(rp.dot(rp) - len*len);

			if (len < s * lookahead_step && d < get_avoidance_factor() * (robot::MAX_RADIUS * 2)) {
				return false;
			}
		}

		if(avoid_ball) {
			const point ballvec = the_ball->position() - start;
			double len = ballvec.dot(direction);
			if (len > 0) {
				double d = sqrt(ballvec.dot(ballvec) - len*len);
				if (len < s * lookahead_step && d < get_avoidance_factor() * (robot::MAX_RADIUS + ball::RADIUS)) {
					//std::cout << "Checked FALSE" << std::endl;
					return false;
				}
			}
		}
	}

	//std::cout << "Checked TRUE" << std::endl;
	return true;
}

