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

void AI::HL::STP::Action::catch_ball(caller_t& ca, World world, Player player, Point target) {
	Point target_line; // the line between the ball and target
	Point catch_pos; // prediction of where the robot should be behind the ball to catch it
	Angle catch_orientation; // The orientation the catcher should have when catching

	do {
		// The angle between the target line, and the line from the robot to the ball
		Angle orientation_diff = (player.position() - world.ball().position()).orientation().angle_diff(target_line.orientation());

		// sets the avoid ball flags based on how close to robot is to the ball (and if it's on the correct side)
		if(orientation_diff < Angle::of_degrees(40)) {
			player.flags(AI::Flags::calc_flags(world.playtype()));
			LOGF_INFO(u8"%1", "NO FLAGS");
		}else if(orientation_diff < Angle::of_degrees(60)) {
			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_TINY);
			LOGF_INFO(u8"%1", "TINY");
		}else {
			player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
			LOGF_INFO(u8"%1", "MEDIUM");
		}

		// If no target is specified, assume we want to shoot
		target = target == Point(-99, -99) ? Evaluation::get_best_shot(world, player) : target;
		target_line = world.ball().position() - target;

		if(world.ball().velocity().lensq() < 1E-3) {
			// ball is either very slow or stopped. prediction algorithms don't work as well so
			// just go behind the ball facing the target
			catch_pos = world.ball().position() + target_line.norm(Robot::MAX_RADIUS * 2);
			catch_orientation = (Evaluation::get_best_shot(world, player) - world.ball().position()).orientation();
		}else {
			catch_pos = Evaluation::baller_catch_position(world, player);
			catch_orientation = (world.ball().position() - catch_pos).orientation();
			//catch_pos = Evaluation::calc_fastest_grab_ball_dest(world, player);

			//from old catch_ball_quick
			//catch_pos = Evaluation::quickest_intercept_position(world, player);
		}

        LOGF_INFO(u8"%1", catch_pos);
		player.move_move(catch_pos, catch_orientation);
		Action::yield(ca);
	} while ((player.position() - catch_pos).lensq() > 0.08 * 0.08 || player.velocity().lensq() > 0.07 * 0.07 ||
				player.orientation().angle_diff(catch_orientation).abs() > Angle::of_degrees(5));

	// how many ticks the player has had the ball for
	int had_ball_for = 0;

	player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_FRIENDLY_DEFENSE
			| AI::Flags::MoveFlags::AVOID_ENEMY_DEFENSE | AI::Flags::MoveFlags::CAREFUL);

    catch_pos = Evaluation::baller_catch_position(world, player);
    Action::dribble(ca, world, player, catch_pos, (world.ball().position() - player.position()).orientation());
}
