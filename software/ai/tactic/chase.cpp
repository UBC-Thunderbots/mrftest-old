#include "ai/tactic/chase.h"
#include "geom/angle.h"

#include <iostream>

namespace {
#warning magic constants
	const double VEL_FACTOR = 0; //origionally 0.5 from byrons code
	const double ACL_FACTOR = 0; //origionally 0.125 from byrons code
	const double STILL_BALL_AMOUNT = 0.005;
	//if two unit vectors dot to give this value then they are the same direction
	const double SAME_DIRECTION = 0.98;
	const double OVERSHOOT_AMOUNT = 0.01;
}

Chase::Chase(Player::Ptr player, World::Ptr world) : Tactic(player), the_world(world), navi(player, world) {
}

void Chase::tick() {
	const Ball::Ptr the_ball(the_world->ball());
	// predict ball position based on velocity and accleration
	//NOTE THAT MOVING BALL MANUALLY WITH CURSOR CAN GIVE VERY LARGE VELOCITY AND ACCELERATION
	//Point pos = the_ball->position() + VEL_FACTOR * the_ball->est_velocity() + ACL_FACTOR * the_ball->est_acceleration();
	const Point pos = the_ball->position();

	//Point player_ball_direction = (the_ball->position()-player->position()).norm();
	//Point ball_direction = (the_ball->est_velocity()).norm();

	// I disagree with the statement below:
	// the robot controller is SUPPOSED to do all that.
	//
	//if we are heading parallel to ball velocity or the ball is still plan to overshoot the ball by a little bit 
	//so that we can power through,beat,etc the opponent to the ball
	/*
	   if( (the_ball->est_velocity()).len() <= STILL_BALL_AMOUNT || player_ball_direction.dot(ball_direction)>SAME_DIRECTION ){
	   pos += player_ball_direction.norm()*OVERSHOOT_AMOUNT;
	   }
	 */

	navi.set_position(pos);
	navi.set_flags(flags);
	navi.tick();
}

