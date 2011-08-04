#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	BoolParam chase_target_enemy("Chase always face toward enemy goal", "STP/Chase", false);
}

void AI::HL::STP::Action::chase(const World &world, Player::Ptr player) {
	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);

	Point target = world.field().enemy_goal();
	if (!shoot_data.blocked) {
		target = shoot_data.target;
	}

	if (chase_target_enemy) {
		player->move(target, Angle::ZERO, Point());
		player->type(AI::Flags::MoveType::CATCH_PIVOT);
	} else {
		player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), Point());
		player->type(AI::Flags::MoveType::CATCH);
	}
	player->prio(AI::Flags::MovePrio::HIGH);
}

void AI::HL::STP::Action::chase(Player::Ptr player, Point target) {
	player->move(target, (target - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::CATCH_PIVOT);
	player->prio(AI::Flags::MovePrio::HIGH);
}

