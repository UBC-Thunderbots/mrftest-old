#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/world.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "ai/hl/world.h"

using namespace AI::HL::STP;
using namespace AI::HL::W;

void AI::HL::STP::Action::intercept_pivot(AI::HL::STP::World world, AI::HL::STP::Player player, const Point target, double radius) {
#warning radius parameter is not used, is undocumented, and nobody except maybe Cheng and James know why it’s here

	//if (Evaluation::behind_ball_within_dist(world, player, target, 0.1, Angle::of_degrees(10.0))){
	//		const Angle ori = (target - player.position()).orientation();

	//if (Evaluation::ball_in_pivot_thresh(world, player)) {
#if 0
	Point target2ball = target-world.ball().position(); 
	Point player2ball = player.position() - world.ball().position();
	double projection = target2ball.dot(player2ball);
	double distance = target2ball.norm().cross(player2ball);
	Angle swing = target2ball.orientation() + Angle::half();
	Point perpen = target2ball.rotate(Angle::quarter()).norm();

	Angle swing_diff = (target2ball.orientation()-player2ball.orientation()+Angle::half()).angle_mod();

	std::cout << "swing diff " << swing_diff.to_degrees() << std::endl;
	std::cout << "projection is " << projection << std::endl;
	// decide if we are infront or behind the ball
	if( (swing_diff.abs() >= Angle::quarter()*0.5) &&( player2ball.len() >= 0.3) ){//projection > -Robot::MAX_RADIUS*2  ){
		//we want to retreat
		//player.mp_catch(target);// need to probably pass down the target too
		#warning potentially undermines movement primitive performance to let destination depend on current robot position
		std::cout << "retreat" << std::endl;
//		player.mp_move(world.ball().position() + (-target2ball).norm()*1.0  + (perpen)*0.4 ,target2ball.orientation());
	// decide if we are infront of ball
	} else if( projection > 0) {
		std::cout << "going side ways " << std::endl;
		player.mp_move(world.ball().position() + (perpen)*0.4, target2ball.orientation());
		std::cout << "orient_retreat " << ori_diff << std::endl;
	// decide if we need to orient to ball
	} else if( swing_diff.abs() >= Angle::of_degrees(30) && player2ball.len() >= 0.3) {
		std::cout << "angle is " << swing.to_degrees() << std::endl;
		player.mp_pivot(world.ball().position(), (-target2ball).orientation());
		std::cout << "turn around" << std::endl;
	// looks like position is pretty good move forward then
	} */else {
		player.mp_shoot(world.ball().position(), (target2ball).orientation(), false, BALLBALL_MAX_SPEED);
		std::cout << "orient_shoot " << ori_diff << std::endl;
		std::cout << "shooting" <<std::endl;
	}
	/*if (Evaluation::ball_in_pivot_thresh(world, player)) {

		//std::cout << "pivot is used" << std::endl;
	//	pivot(world, player, target);
	} else {
		//std::cout << "intercept is used" << std::endl;
		//intercept(player, target);

		//if( world.ball().velocity().len() > 1.0 ){
			const Angle ori = (target - player.position()).orientation();
			Point dest = -(target - world.ball().position()).norm() * Robot::MAX_RADIUS + world.ball().position();
			player.mp_move(dest, ori);
		//}
	}*/
#elif 1
	Point target2ball = world.ball().position() - target;
	Point player2ball = world.ball().position() - player.position();
	Angle swing_diff = (target2ball.orientation() - player2ball.orientation() + Angle::half()).angle_mod();

	if (swing_diff.abs() <= Angle::of_degrees(20) || (player2ball.len() < 0.25 && swing_diff.abs() <= Angle::quarter())) {
		LOG_INFO(Glib::ustring::compose("Good, GTFI (ball=%1, target=%2, pos=%3, len=%4, swing_diff=%5, shoot orientation=%6)", world.ball().position(), target, player.position(), player2ball.len(), swing_diff, (-target2ball).orientation()));
		player.mp_shoot(world.ball().position(), (-target2ball).orientation(), false, BALL_MAX_SPEED);
	} else if (player2ball.len() < 0.25) {
		// Get further away so we can pivot safely.
		LOG_INFO(Glib::ustring::compose("Too close, GTFO (ball=%1, target=%2, pos=%3, len=%4, move to %5)", world.ball().position(), target, player.position(), player2ball.len(), world.ball().position() - player2ball.norm() * 0.3));
		player.mp_move(world.ball().position() - player2ball.norm() * 0.3, (-target2ball).orientation());
	} else {
		LOG_INFO(Glib::ustring::compose("Pivot the thing (ball=%1, target=%2, pos=%3, len=%4, swing_diff=%5, target orientation=%6)", world.ball().position(), target, player.position(), player2ball.len(), swing_diff, (-target2ball).orientation()));
		player.mp_pivot(world.ball().position(), (-target2ball).orientation());
	}
#endif
}


void AI::HL::STP::Action::intercept_pivot(AI::HL::STP::World world, AI::HL::STP::Player player, const Point target, double radius, double ball_velocity) {

	//if (Evaluation::behind_ball_within_dist(world, player, target, 0.1, Angle::of_degrees(10.0))){
	//		const Angle ori = (target - player.position()).orientation();

	//if (Evaluation::ball_in_pivot_thresh(world, player)) {
#if 0
	Point target2ball = target-world.ball().position();
	Point player2ball = player.position() - world.ball().position();
	double projection = target2ball.dot(player2ball);
	double distance = target2ball.norm().cross(player2ball);
	Angle swing = target2ball.orientation() + Angle::half();
	Point perpen = target2ball.rotate(Angle::quarter()).norm();

	Angle swing_diff = (target2ball.orientation()-player2ball.orientation()+Angle::half()).angle_mod();

	std::cout << "swing diff " << swing_diff.to_degrees() << std::endl;
	std::cout << "projection is " << projection << std::endl;
	// decide if we are infront or behind the ball
	if( (swing_diff.abs() >= Angle::quarter()*0.5) &&( player2ball.len() >= 0.3) ){//projection > -Robot::MAX_RADIUS*2  ){
		//we want to retreat
		//player.mp_catch(target);// need to probably pass down the target too
		#warning potentially undermines movement primitive performance to let destination depend on current robot position
		std::cout << "retreat" << std::endl;
//		player.mp_move(world.ball().position() + (-target2ball).norm()*1.0  + (perpen)*0.4 ,target2ball.orientation());
	// decide if we are infront of ball
	} else if( projection > 0) {
		std::cout << "going side ways " << std::endl;
		player.mp_move(world.ball().position() + (perpen)*0.4, target2ball.orientation());
		std::cout << "orient_retreat " << ori_diff << std::endl;
	// decide if we need to orient to ball
	} else if( swing_diff.abs() >= Angle::of_degrees(30) && player2ball.len() >= 0.3) {
		std::cout << "angle is " << swing.to_degrees() << std::endl;
		player.mp_pivot(world.ball().position(), (-target2ball).orientation());
		std::cout << "turn around" << std::endl;
	// looks like position is pretty good move forward then
	} */else {
		player.mp_shoot(world.ball().position(), (target2ball).orientation(), false, BALL_BALL_MAX_SPEED);
		std::cout << "orient_shoot " << ori_diff << std::endl;
		std::cout << "shooting" <<std::endl;
	}
	/*if (Evaluation::ball_in_pivot_thresh(world, player)) {

		//std::cout << "pivot is used" << std::endl;
	//	pivot(world, player, target);
	} else {
		//std::cout << "intercept is used" << std::endl;
	//	intercept(player, target);

		//if( world.ball().velocity().len() > 1.0 ){
			const Angle ori = (target - player.position()).orientation();
			Point dest = -(target - world.ball().position()).norm() * Robot::MAX_RADIUS + world.ball().position();
			player.mp_move(dest, ori);
		//}
	}*/
#elif 1
	Point target2ball = world.ball().position() - target;
	Point player2ball = world.ball().position() - player.position();
	Angle swing_diff = (target2ball.orientation() - player2ball.orientation() + Angle::half()).angle_mod();

	if (swing_diff.abs() <= Angle::of_degrees(20) || (player2ball.len() < 0.25 && swing_diff.abs() <= Angle::quarter())) {
		LOG_INFO(Glib::ustring::compose("Good, GTFI (ball=%1, target=%2, pos=%3, len=%4, swing_diff=%5, shoot orientation=%6)", world.ball().position(), target, player.position(), player2ball.len(), swing_diff, (-target2ball).orientation()));
		player.mp_shoot(world.ball().position(), (-target2ball).orientation(), false, BALL_MAX_SPEED);
	} else if (player2ball.len() < 0.25) {
		// Get further away so we can pivot safely.
		LOG_INFO(Glib::ustring::compose("Too close, GTFO (ball=%1, target=%2, pos=%3, len=%4, move to %5)", world.ball().position(), target, player.position(), player2ball.len(), world.ball().position() - player2ball.norm() * 0.3));
		player.mp_move(world.ball().position() - player2ball.norm() * 0.3, (-target2ball).orientation());
	} else {
		LOG_INFO(Glib::ustring::compose("Pivot the thing (ball=%1, target=%2, pos=%3, len=%4, swing_diff=%5, target orientation=%6)", world.ball().position(), target, player.position(), player2ball.len(), swing_diff, (-target2ball).orientation()));
		player.mp_pivot(world.ball().position(), (-target2ball).orientation());
	}
#endif
}



void AI::HL::STP::Action::pivot(AI::HL::STP::World world, AI::HL::STP::Player player, const Point target, const double radius) {
	// set the destination point to be just behind the ball in the correct direction at the offset distance
	Point dest = -(target - world.ball().position()).norm() * radius + world.ball().position();
	player.mp_pivot(dest, (world.ball().position() - dest).orientation());
	player.type(AI::Flags::MoveType::PIVOT);
	player.prio(AI::Flags::MovePrio::HIGH);
}

