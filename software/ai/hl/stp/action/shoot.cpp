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

using namespace AI::HL::STP;

void AI::HL::STP::Action::shoot_goal(caller_t& ca, World world, Player player, bool chip) {
	Point shot = Evaluation::get_best_shot(world, player);

	shoot_target(ca, world, player, shot, BALL_MAX_SPEED, chip);
}

void AI::HL::STP::Action::shoot_target(caller_t& ca, World world, Player player, Point target, double velocity, bool chip) {
	const Angle orient = (target - player.position()).orientation();

	// wait for chicker to be ready
	while (!player.chicker_ready()) Action::yield(ca);

	// ram the ball
	Primitives::Shoot shoot(player, world.ball().position() + (world.ball().position() - player.position()).norm(0.05), orient, velocity, chip);


	for (int i = 0; i < 10; i++) Action::yield(ca);		//this is a hack from RoboCup used for stabilization ( DO NOT CHANGE )

	player.clear_prims();
}

void AI::HL::STP::Action::catch_and_shoot_target(caller_t& ca, World world, Player player, Point target, double velocity, bool chip) {
	while(!player.has_ball()) {
		#warning TODO this doesn't actually look at the target
		#warning TODO this only works for stopped balls
		#warning TODO change this while loop after action rewrite

		catch_ball(ca, world, player, target);
	}

	shoot_target(ca, world, player, target, velocity, chip);
}

void AI::HL::STP::Action::catch_and_shoot_goal(caller_t& ca, World world, Player player, bool chip) {
	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);

	catch_and_shoot_target(ca, world, player, shoot_data.target, BALL_MAX_SPEED, chip);
}
