#include "ai/flags.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/move.h"

using namespace AI::HL::STP;

void get_behind_ball(caller_t& ca, World world, Player player, Point target) {
	double playerproj = Geom::proj_len(Geom::Seg(world.ball().position(), target), player.position());
	double behinddist = Robot::MAX_RADIUS + 0.18;
	Point behind_ball = world.ball().position() + (world.ball().position() - target).norm(behinddist);

	if (playerproj > -behinddist + 0.10) {
		Point dest1 = behind_ball + (world.ball().position() - target).norm(0.3).rotate(Angle::quarter());
		Point dest2 = behind_ball + (world.ball().position() - target).norm(0.3).rotate(-Angle::quarter());

		// Go to the closest position to the player out of the two sideways points from the behind-ball point
		Point dest = (dest1 - player.position()).lensq() > (dest2 - player.position()).lensq() ? dest2 : dest1;

		// Get behind the ball (relative to the target)
		Action::move(ca, world, player, dest, (target - world.ball().position()).orientation());
	}
	else {
		Point dest = behind_ball;
		player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
		player.move_move(dest, (target - world.ball().position()).orientation());
	}
}

void AI::HL::STP::Action::shoot_goal(caller_t& ca, World world, Player player, bool chip) {
	Point shot = Evaluation::get_best_shot(world, player);

	shoot_target(ca, world, player, shot, BALL_MAX_SPEED, chip);
}

void AI::HL::STP::Action::shoot_target(caller_t& ca, World world, Player player, Point target, double velocity, bool chip) {
	AI::Flags::MoveFlags playerFlags = player.flags();
	//player.unset_flags(AI::Flags::MoveFlags::AVOID_BALL_MEDIUM | AI::Flags::MoveFlags::AVOID_BALL_TINY);
	if (Evaluation::in_shoot_position(world, player, target)) {
		const Angle orient = (target - world.ball().position()).orientation();
		player.move_shoot(world.ball().position(), orient, velocity, chip);
	} else {
		// Get behind the ball without hitting it
		// TODO: account for slowly moving ball (fast ball handled by catch)
	//	player.set_flags(playerFlags);
		get_behind_ball(ca, world, player, target);
	}
	//player.set_flags(playerFlags);
}

void AI::HL::STP::Action::catch_and_shoot_target(caller_t& ca, World world, Player player, Point target, double velocity, bool chip) {
	//TODO: add proper catch action

	/*
	if(world.ball().velocity().len() > 1.5){
		//TODO: make catch_ball not wait
		catch_ball(ca, world, player, target);
	}else
	*/
	if (Evaluation::in_shoot_position(world, player, target)) {
		shoot_target(ca, world, player, target, velocity, chip);
	} else {
		// Get behind the ball without hitting it
		// TODO: account for slowly moving ball (fast ball handled by catch)
		get_behind_ball(ca, world, player, target);
	}
}

void AI::HL::STP::Action::catch_and_shoot_goal(caller_t& ca, World world, Player player, bool chip) {
	//TODO: remove the while true
	while(true){
		Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);
		catch_and_shoot_target(ca, world, player, shoot_data.target, BALL_MAX_SPEED, chip);
	}
}
