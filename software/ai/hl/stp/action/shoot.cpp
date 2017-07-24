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


static bool in_shoot_position(Point pos, Point ball, Point target){
	if((pos - ball).len() > 1.0) return false;
	else if(((ball - pos).orientation() - (target - ball).orientation()).abs() > Angle::of_degrees(25.0)) return false;
	return true;
}

void AI::HL::STP::Action::shoot_goal(caller_t& ca, World world, Player player, bool chip, bool should_wait) {
	Point shot = Evaluation::get_best_shot(world, player);

	shoot_target(ca, world, player, shot, BALL_MAX_SPEED, chip, should_wait);
}

void AI::HL::STP::Action::shoot_target(caller_t& ca, World world, Player player, Point target, double velocity, bool chip, bool should_wait) {
	const Angle orient = (target - world.ball().position()).orientation();

	// wait for chicker to be ready
	while (!player.chicker_ready()) Action::yield(ca);

	// ram the ball

	player.move_shoot(world.ball().position(), orient, velocity, chip);
//	player.move_move(world.ball().position() + (target - world.ball().position()).norm(0.05), orient, 0);
    //player.move_shoot(Point(0,0), Angle::zero(), velocity, chip);
    LOGF_INFO(u8"%1", world.ball().position() + (world.ball().position() - player.position()).norm(0.05));
    if(should_wait) Action::wait_shoot(ca, player);
}

void AI::HL::STP::Action::catch_and_shoot_target(caller_t& ca, World world, Player player, Point target, double velocity, bool chip, bool should_wait) {
	bool done = false;
	do{
		/*if(world.ball().velocity().len() > 1.5){
			//TODO: make catch_ball not wait
			catch_ball(ca, world, player, target);
		}else 
*/
		if(in_shoot_position(player.position(), world.ball().position(), target)){
			LOG_INFO("DONE CATCH. SHOOTING NOW");
			shoot_target(ca, world, player, target, velocity, chip, false);
		}else{
			// Get behind the ball without hitting it
			// TODO: account for slowly moving ball (fast ball handled by catch)
			Point dest = world.ball().position() + (world.ball().position() - target).norm(0.21);
			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
			move_rrt(ca, world, player, dest, (target - world.ball().position()).orientation(), false);
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
