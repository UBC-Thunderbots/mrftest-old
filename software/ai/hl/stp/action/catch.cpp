#include "ai/flags.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/intercept.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "geom/util.h"
#include "geom/point.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::just_catch_ball(caller_t& ca, World world, Player player) {
    player.move_catch(Angle(), 0, 0); 
    while (!player.has_ball()){
        Action::yield(ca); 
    }
}

void AI::HL::STP::Action::catch_ball(caller_t& ca, World world, Player player, Point target) {
	Point target_line; // the line between the ball and target
	Point catch_pos; // prediction of where the robot should be behind the ball to catch it
	Angle catch_orientation; // The orientation the catcher should have when catching

    LOGF_INFO(u8"%1", "catchchin");
	do {
		// The angle between the target line, and the line from the robot to the ball
		Angle orientation_diff = (player.position() - world.ball().position()).orientation().angle_diff(target_line.orientation());

		// sets the avoid ball flags based on how close to robot is to the ball (and if it's on the correct side)
		if(orientation_diff < Angle::of_degrees(20)) {
			player.flags(AI::Flags::calc_flags(world.playtype()));
			LOGF_INFO(u8"%1", "NO FLAGS");
		}else if(orientation_diff < Angle::of_degrees(30)) {
			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_TINY);
			LOGF_INFO(u8"%1", "TINY");
		}else {
			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
			LOGF_INFO(u8"%1", "MEDIUM");
		}

		// If no target is specified, assume we want to shoot
		target = target == Point(-99, -99) ? Evaluation::get_best_shot(world, player) : target;
		target_line = world.ball().position() - target;

		if(world.ball().velocity().lensq() < 0.05) {
			LOG_INFO("SITTING STILL");
			// ball is either very slow or stopped. prediction algorithms don't work as well so
			// just go behind the ball facing the target
			catch_pos = world.ball().position() + target_line.norm(Robot::MAX_RADIUS * 3);
			catch_orientation = (Evaluation::get_best_shot(world, player) - world.ball().position()).orientation();
		}else {
			LOG_INFO("MOVING BALL");
//			catch_pos = Evaluation::baller_catch_position(world, player);
			catch_pos = Evaluation::calc_fastest_grab_ball_dest(world, player);
			catch_orientation = (world.ball().position() - catch_pos).orientation();

			//from old catch_ball_quick
			//catch_pos = Evaluation::quickest_intercept_position(world, player);
		}

        Action::move_rrt(ca, world, player, catch_pos, catch_orientation, false);

		Action::yield(ca);
		LOGF_INFO("%1, %2, %3", (player.position() - catch_pos).lensq() > 0.08 * 0.08, player.velocity().lensq() > 0.07 * 0.07,
				player.orientation().angle_diff(catch_orientation).abs() > Angle::of_degrees(5));
	} while ((player.position() - catch_pos).lensq() > 0.08 * 0.08 || player.velocity().lensq() > 0.07 * 0.07 ||
				player.orientation().angle_diff(catch_orientation).abs() > Angle::of_degrees(8));

	LOG_INFO("DONE INITIAL APPROACH LOOP");

	// Make contact with the ball now that the robot is in its path
	// how many ticks the player has had the ball for
//	int had_ball_for = 0;
//
//	player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_FRIENDLY_DEFENSE
//			| AI::Flags::MoveFlags::AVOID_ENEMY_DEFENSE | AI::Flags::MoveFlags::CAREFUL);
//
//    catch_pos = Evaluation::baller_catch_position(world, player);
//    LOGF_INFO(u8"catching dribble to %1", catch_pos);
//    Action::move_slp(ca, world, player, catch_pos, (world.ball().position() - player.position()).orientation());
}
