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

void AI::HL::STP::Action::shoot_goal(caller_t& ca, World world, Player player, bool chip, bool should_wait) {
	Point shot = Evaluation::get_best_shot(world, player);

	shoot_target(ca, world, player, shot, BALL_MAX_SPEED, chip, should_wait);
}

void AI::HL::STP::Action::shoot_target(caller_t& ca, World world, Player player, Point target, double velocity, bool chip, bool should_wait) {
	bool done = false;
	if (Evaluation::in_shoot_position(world, player, target)) {
		const Angle orient = (target - world.ball().position()).orientation();
		player.move_shoot(world.ball().position(), orient, velocity, chip);
		LOG_INFO("DONE CATCH. SHOOTING NOW");
	} else {
		// Get behind the ball without hitting it
		// TODO: account for slowly moving ball (fast ball handled by catch)
		LOG_INFO("NOT IN POSSITION");
		Point dest = world.ball().position() + (world.ball().position() - target).norm(0.21);
		player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
		player.move_move(dest, (target - world.ball().position()).orientation());
	}
}

void AI::HL::STP::Action::catch_and_shoot_target(caller_t& ca, World world, Player player, Point target, double velocity, bool chip, bool should_wait) {
	bool done = false;
	do{

		//TODO: add proper catch action

		/*if(world.ball().velocity().len() > 1.5){
			//TODO: make catch_ball not wait
			catch_ball(ca, world, player, target);
		}else
*/
		if(Evaluation::in_shoot_position(world, player, target)){
			shoot_target(ca, world, player, target, velocity, chip, false);
		}else{
			// Get behind the ball without hitting it
			// TODO: account for slowly moving ball (fast ball handled by catch)
			Point dest = world.ball().position() + (world.ball().position() - target).norm(0.21);
			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
			move(ca, world, player, dest, (target - world.ball().position()).orientation(), false);
		}
		if(should_wait) yield(ca);
	}while(!done && should_wait);
}

void AI::HL::STP::Action::catch_and_shoot_goal(caller_t& ca, World world, Player player, bool chip, bool should_wait) {
	//TODO: remove the while true
	while(true){
		Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);
		catch_and_shoot_target(ca, world, player, shoot_data.target, BALL_MAX_SPEED, chip, false);
		yield(ca);
	}
}
