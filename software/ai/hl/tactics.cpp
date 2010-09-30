#include "ai/flags.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"

using namespace AI::HL::W;

#warning not sure what to do with the play flags

void AI::HL::Tactics::chase(World &world, Player::Ptr player) {
	player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
}

void AI::HL::Tactics::shoot(World &world, Player::Ptr player, const bool force) {
	if (!player->has_ball()) {
		chase(world, player);
		return;
	}

	std::pair<Point, double> target = AI::HL::Util::calc_best_shot(world, player);
	if (target.second == 0) { // blocked
		if (force) {
			// TODO: perhaps do a reduced radius calculation
			target.first = world.field().enemy_goal();
			AI::HL::Tactics::shoot(world, player, target.first);
		} else { // just aim at the enemy goal
			player->move(player->position(), (world.field().enemy_goal() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
		}
	} else {
		// call the other shoot function with the specified target
		AI::HL::Tactics::shoot(world, player, target.first);
	}
}

void AI::HL::Tactics::shoot(World &world, Player::Ptr player, const Point target) {
	if (!player->has_ball()) {
		chase(world, player);
		return;
	}

	const double ori_target = (target - player->position()).orientation();
	const double ori_diff = fabs(player->orientation() - ori_target);

	if (ori_diff > AI::HL::Util::shoot_accuracy * M_PI / 180.0) { // aim
		player->move(player->position(), ori_target, AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
		return;
	}

	// shoot!
	if (player->chicker_ready_time() == 0) {
#warning max power kick for now
		player->kick(1.0);
	}
}

