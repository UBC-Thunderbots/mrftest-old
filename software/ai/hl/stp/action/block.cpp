#include "ai/hl/stp/action/block.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam block_threshold("block threshold distance in terms of robot radius", "STP/Action/block", 3.0, 2.0, 4.0);

}

void AI::HL::STP::Action::block(const World &world, Player::Ptr player, Robot::Ptr robot) {
	Point dirToGoal = (world.field().friendly_goal() - robot->position()).norm();
	player->move(robot->position() + (block_threshold * Robot::MAX_RADIUS * dirToGoal), (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);

}

void AI::HL::STP::Action::block_pass(const World &world, Player::Ptr player, Robot::Ptr robot) {
	Point dirToBall = (world.ball().position() - robot->position()).norm();
	player->move(robot->position() + (block_threshold * Robot::MAX_RADIUS * dirToBall), (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
	player->prio(AI::Flags::MovePrio::MEDIUM);

}

