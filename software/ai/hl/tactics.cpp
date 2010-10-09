#include "ai/flags.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"

using namespace AI::HL::W;

void AI::HL::Tactics::chase(World &world, Player::Ptr player, const unsigned int flags) {
	player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
}

void AI::HL::Tactics::shoot(World &world, Player::Ptr player, const unsigned int flags, const bool force) {
	if (!player->has_ball()) {
		chase(world, player, flags);
		return;
	}

	std::pair<Point, double> target = AI::HL::Util::calc_best_shot(world, player);
	if (target.second == 0) { // blocked
		if (force) {
			// TODO: perhaps do a reduced radius calculation
			target.first = world.field().enemy_goal();
			shoot(world, player, flags, target.first);
		} else { // just aim at the enemy goal
			player->move(player->position(), (world.field().enemy_goal() - player->position()).orientation(), flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
		}
	} else {
		// call the other shoot function with the specified target
		AI::HL::Tactics::shoot(world, player, flags, target.first);
	}
}

void AI::HL::Tactics::shoot(World &world, Player::Ptr player, const unsigned int flags, const Point target) {
	if (!player->has_ball()) {
		chase(world, player, flags);
		return;
	}

	const double ori_target = (target - player->position()).orientation();
	const double ori_diff = fabs(player->orientation() - ori_target);

	if (ori_diff > AI::HL::Util::shoot_accuracy * M_PI / 180.0) { // aim
		player->move(player->position(), ori_target, flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
		return;
	}

	// shoot!
	if (player->chicker_ready_time() == 0) {
#warning max power kick for now
		player->kick(1.0);
	}
}

void AI::HL::Tactics::free_move(AI::HL::W::World &world, AI::HL::W::Player::Ptr player, const Point p) {
	// no flags
	player->move(p, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
}

