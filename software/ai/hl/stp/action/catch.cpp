#include "ai/flags.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/legacy_shoot.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/legacy_move.h"
#include "ai/hl/stp/action/legacy_dribble.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/intercept.h"
#include "geom/util.h"
#include "geom/point.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::catch_stopped_ball(caller_t& ca, World world, Player player) {
	bool approach_armed = false;
	int had_ball_for = 0;

	while (true) {
		if (player.has_ball()) {
			had_ball_for++;
		}
		else {
			had_ball_for = 0;
		}

		Point target = Evaluation::get_best_shot(world, player);
		double inner_radius = Robot::MAX_RADIUS + 0.10;
		Point proj_ball = Evaluation::baller_catch_position(world, player);
		Point behind_ball = proj_ball - (target - proj_ball).norm() * inner_radius;
		Point pp = player.position();

		double r = (proj_ball - pp).len();

		/*
		if (r > 0.5) {
			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
			Action::move(world, player, behind_ball, (target - behind_ball).orientation());
			approach_armed = false;
		}
		else */ if (r > 0.3) {
			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
			Action::move(world, player, behind_ball, (target - behind_ball).orientation());
			approach_armed = false;
		}
		else if (((behind_ball - pp).len() > 0.02 || player.velocity().len() > 0.03) && !approach_armed) {
			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_TINY | AI::Flags::MoveFlags::CAREFUL);
			Action::move(world, player, behind_ball, (target - pp).orientation());
		}
		else if (had_ball_for < 5) {
			approach_armed = true;

			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::CAREFUL);
			// Action::shoot_target(world, player, target);
			// Action::move(world, player, proj_ball, (target - player.position()).orientation());
			Action::dribble(world, player, proj_ball, (target - player.position()).orientation());
		}
		else {
			break;
		}

		Action::yield(ca);
	}
}

void AI::HL::STP::Action::catch_ball(caller_t& ca, World world, Player player, Point target) {
	player.flags(player.flags() | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);

	Primitives::Primitive::Ptr move;
	Point behind_ball;
	Point proj_ball;
	do {
		proj_ball = Evaluation::baller_catch_position(world, player);
		behind_ball = proj_ball - (target - proj_ball).norm() * (Robot::MAX_RADIUS + 0.05);
		move.reset();
		move = Primitives::Primitive::Ptr(new Primitives::Move
				(player, behind_ball, (proj_ball - behind_ball).orientation(), (player.position() - behind_ball).len() * 5));
		Action::yield(ca);
	} while ((player.position() - behind_ball).lensq() > 0.08 * 0.08 || player.velocity().lensq() > 0.07 * 0.07 ||
			(player.orientation() - (proj_ball - player.position()).orientation()).abs() > Angle::of_degrees(8));
	move.reset();

	player.flags(AI::Flags::MoveFlags::AVOID_FRIENDLY_DEFENSE | AI::Flags::MoveFlags::AVOID_ENEMY_DEFENSE);
	do {
		proj_ball = Evaluation::baller_catch_position(world, player);
		move.reset();
		move = Primitives::Primitive::Ptr(new Primitives::Move
				(player, proj_ball, (proj_ball - player.position()).orientation(), 1));
		Action::yield(ca);
	} while (!player.has_ball());
	move.reset();
}

void AI::HL::STP::Action::catch_ball_quick(caller_t& ca, World world, Player player) {
	//player.flags(player.flags() | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);

	Primitives::Primitive::Ptr move;
	Point behind_ball;
	Point intercept_pos;
	double ball_vel_threshold = 0.001;

	do {
		//IF THE BALL IS MOVING
		if(world.ball().velocity().len() > ball_vel_threshold) {

			intercept_pos = Evaluation::quickest_intercept_position(world, player);
			behind_ball = intercept_pos + (world.ball().velocity().norm(Robot::MAX_RADIUS * 2));

			//If the robot is in the ball's path and close
			if( (player.position() - closest_lineseg_point(player.position(), world.ball().position(), world.ball().position() + world.ball().velocity().norm(10.0))).len() < Robot::MAX_RADIUS/1.5 &&
					(player.position() - world.ball().position()).len() < Robot::MAX_RADIUS * 7) {

				player.flags(AI::Flags::MoveFlags::AVOID_FRIENDLY_DEFENSE | AI::Flags::MoveFlags::AVOID_ENEMY_DEFENSE);
				move.reset();
				move = Primitives::Primitive::Ptr(new Primitives::Move
						(player, intercept_pos, (world.ball().position() - player.position()).orientation(), 1));//face ball
			}
			else {
				player.flags(player.flags() | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
				move.reset();
				move = Primitives::Primitive::Ptr(new Primitives::Move
						(player, behind_ball, (world.ball().position() - behind_ball).orientation(), (player.position() - behind_ball).len() * 5));//face ball
			}
		}
		else {//IF THE BALL IS SLOW/STOPPED

			intercept_pos = Evaluation::quickest_intercept_position(world, player);
			behind_ball = intercept_pos + (world.ball().position() - world.field().enemy_goal()).norm(Robot::MAX_RADIUS*1.5);

			//STEP 1, get behind ball
			if((player.position() - behind_ball).len() > Robot::MAX_RADIUS*1.5) {
				player.flags(player.flags() | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
				move.reset();
				move = Primitives::Primitive::Ptr(new Primitives::Move
					(player, behind_ball, (world.field().enemy_goal() - player.position()).orientation(), 1));//face goal
			}
			//STEP 2, approach
			else {
				player.flags(AI::Flags::MoveFlags::AVOID_FRIENDLY_DEFENSE | AI::Flags::MoveFlags::AVOID_ENEMY_DEFENSE);
				move.reset();
				move = Primitives::Primitive::Ptr(new Primitives::Move
						(player, intercept_pos, (world.field().enemy_goal() - player.position()).orientation(), 1));//face goal
			}
		}

		Action::yield(ca);
	}while (!player.has_ball());
	move.reset();

}
