#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Evaluation::BallThreat;

namespace {
	DoubleParam lone_goalie_dist("Lone Goalie: distance to goal post (m)", "STP/Action/Goalie", 0.30, 0.05, 1.0);
}

void AI::HL::STP::Action::lone_goalie(const World &world, Player::Ptr player) {
	// Patrol
	const Point default_pos = Point(-0.45 * world.field().length(), 0);
	const Point centre_of_goal = world.field().friendly_goal();
	Point target = world.ball().position() - centre_of_goal;
	target = target * (lone_goalie_dist / target.len());
	target += centre_of_goal;
	if (target.x < -world.field().length() / 2 + 0.2) { // avoid going inside the goal
		target.x = -world.field().length() / 2 + 0.2;
	}
	player->move(target, (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
	player->prio(AI::Flags::MovePrio::MEDIUM);

	goalie_move(world, player, target);
}

void AI::HL::STP::Action::goalie_move(const World &world, Player::Ptr player, Point dest) {
	// if ball is inside the defense area, must repel!
	if (AI::HL::Util::point_in_friendly_defense(world.field(), world.ball().position())) {
		repel(world, player);
		return;
	}

	// check if ball is heading towards our goal
	if (Evaluation::ball_on_net(world)) {
		// goalie block position
		Point goal_pos = Evaluation::goalie_shot_block(world);

		player->move(goal_pos, (world.ball().position() - player->position()).orientation(), Point());
		player->type(AI::Flags::MoveType::RAM_BALL);
		player->prio(AI::Flags::MovePrio::HIGH);
	} else {
		player->move(dest, (world.ball().position() - player->position()).orientation(), Point());
		player->type(AI::Flags::MoveType::NORMAL);
		player->prio(AI::Flags::MovePrio::MEDIUM);
	}
}

