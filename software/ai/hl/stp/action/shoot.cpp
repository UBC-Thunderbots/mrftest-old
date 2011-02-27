#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/actions.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"

bool AI::HL::STP::Action::shoot(const World &world, Player::Ptr player, const unsigned int flags, const bool force) {
	std::pair<Point, double> target = AI::HL::Util::calc_best_shot(world, player);

	if (!player->has_ball()) {
		if (target.second == 0) {
			// just grab the ball, don't care about orientation
			chase(world, player, flags);
		} else {
			// orient towards the enemy goal area
			player->move(world.ball().position(), (world.field().enemy_goal() - player->position()).orientation(), flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
		}
		return false;
	}

	if (target.second == 0) { // bad news, we are blocked
		if (force) {
			// TODO: perhaps do a reduced radius calculation
			target.first = world.field().enemy_goal();
			return shoot(world, player, target.first, flags);
		} else { // just aim at the enemy goal
			player->move(player->position(), (world.field().enemy_goal() - player->position()).orientation(), flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
		}
		return false;
	}

	// call the other shoot function with the specified target
	return AI::HL::STP::Action::shoot(world, player, target.first, flags);
}

bool AI::HL::STP::Action::shoot(const World &world, Player::Ptr player, const Point target, const unsigned int flags, const bool force) {
	const double ori_target = (target - player->position()).orientation();

	if (!player->has_ball()) {
		// chase(world, player, flags);
		player->move(world.ball().position(), ori_target, flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
		return false;
	}

	// const double ori_diff = std::fabs(player->orientation() - ori_target);

	// aim
	player->move(player->position(), ori_target, flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);

	// ignoring accuracy, comment this out for now so that we'll shoot more
	// if (ori_diff > AI::HL::STP::Util::shoot_accuracy * M_PI / 180.0) { // aim
	// return;
	// }

	// shoot!
	double kick_power = 10.0;
	if (player->chicker_ready_time() == 0) {
		player->kick(kick_power);
		return true;
	}

	return false;
}

